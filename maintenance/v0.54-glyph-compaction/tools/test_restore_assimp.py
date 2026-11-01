"""Small offline failure/atomicity checks for pinned dependency restoration."""
import copy
import io
from pathlib import Path
import tarfile
import tempfile
import unittest
from restore_assimp_ci import digest, restore


class RestoreChecks(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.overlay = self.root / 'overlay'
        self.overlay.mkdir()
        (self.overlay / 'config.txt').write_bytes(b'local patch')
        self.destination = self.root / 'vendor'
        self.archive = self.root / 'source.tar.gz'
        self.make_archive(b'upstream')
        self.manifest = {
            'archive_prefix': 'source/', 'archive_sha256': digest(self.archive.read_bytes()),
            'overlays': ['config.txt'],
            'files': {'a.txt': digest(b'upstream'), 'config.txt': digest(b'local patch')}}

    def make_archive(self, data, symlink=False, omit=False):
        with tarfile.open(self.archive, 'w:gz') as package:
            if omit:
                return
            member = tarfile.TarInfo('source/a.txt')
            if symlink:
                member.type = tarfile.SYMTYPE
                member.linkname = '../outside'
                package.addfile(member)
            else:
                member.size = len(data)
                package.addfile(member, io.BytesIO(data))

    def call(self, manifest=None, archive=None):
        return restore(manifest or self.manifest, self.overlay, self.destination, archive or self.archive)

    def test_roundtrip(self):
        self.assertEqual(self.call(), 2)
        self.assertEqual((self.destination / 'a.txt').read_bytes(), b'upstream')
        self.assertEqual((self.destination / 'config.txt').read_bytes(), b'local patch')

    def test_complete_needs_no_archive(self):
        self.call()
        self.assertEqual(self.call(archive=self.root / 'missing'), 2)

    def test_archive_hash_failure_leaves_no_destination(self):
        self.archive.write_bytes(b'wrong')
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.destination.exists())

    def test_file_hash_failure(self):
        self.make_archive(b'changed')
        self.manifest['archive_sha256'] = digest(self.archive.read_bytes())
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.destination.exists())

    def test_overlay_hash_failure(self):
        (self.overlay / 'config.txt').write_bytes(b'tampered')
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.destination.exists())

    def test_missing_member(self):
        self.make_archive(b'', omit=True)
        self.manifest['archive_sha256'] = digest(self.archive.read_bytes())
        with self.assertRaises(KeyError):
            self.call()
        self.assertFalse(self.destination.exists())

    def test_local_modification_preserved(self):
        self.destination.mkdir()
        (self.destination / 'a.txt').write_bytes(b'personal edit')
        with self.assertRaises(ValueError):
            self.call()
        self.assertEqual((self.destination / 'a.txt').read_bytes(), b'personal edit')

    def test_unlisted_file_preserved(self):
        self.destination.mkdir()
        (self.destination / 'notes.txt').write_bytes(b'keep')
        with self.assertRaises(ValueError):
            self.call()
        self.assertEqual((self.destination / 'notes.txt').read_bytes(), b'keep')

    def test_unsafe_names(self):
        for name in ['../escape', '/absolute', 'C:drive', 'bad\\name', 'x/./y']:
            with self.subTest(name=name):
                manifest = copy.deepcopy(self.manifest)
                manifest['files'][name] = digest(b'x')
                with self.assertRaises(ValueError):
                    self.call(manifest)
        self.assertFalse(self.destination.exists())

    def test_archive_symlink_refused(self):
        self.make_archive(b'', symlink=True)
        self.manifest['archive_sha256'] = digest(self.archive.read_bytes())
        with self.assertRaises(ValueError):
            self.call()
        self.assertFalse(self.destination.exists())

    def test_partial_valid_tree_restored(self):
        self.destination.mkdir()
        (self.destination / 'a.txt').write_bytes(b'upstream')
        self.assertEqual(self.call(), 2)
        self.assertEqual((self.destination / 'config.txt').read_bytes(), b'local patch')

    def test_failed_stage_preserves_partial_tree(self):
        self.destination.mkdir()
        (self.destination / 'a.txt').write_bytes(b'upstream')
        (self.overlay / 'config.txt').write_bytes(b'bad')
        with self.assertRaises(ValueError):
            self.call()
        self.assertEqual((self.destination / 'a.txt').read_bytes(), b'upstream')
        self.assertFalse((self.destination / 'config.txt').exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
