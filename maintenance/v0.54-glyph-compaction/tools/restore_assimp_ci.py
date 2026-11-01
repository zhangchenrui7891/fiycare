"""Restore the exact bundled Assimp subset for source-only CI; no runtime assets."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import tarfile
import tempfile
import urllib.request


def digest(data):
    return hashlib.sha256(data).hexdigest()


def restore(manifest, overlay, destination, archive):
    files = manifest['files']
    if not files:
        raise ValueError('Empty Assimp source manifest')
    for name in files:
        path = PurePosixPath(name)
        if (path.is_absolute() or '..' in path.parts or ':' in name or
                '\\' in name or not name or path.as_posix() != name):
            raise ValueError('Unsafe dependency path')
    missing = []
    for name, expected in files.items():
        path = destination / name
        if path.is_symlink():
            raise ValueError('Dependency symlink refused')
        if path.exists():
            if not path.is_file() or digest(path.read_bytes()) != expected:
                raise ValueError('Locally modified dependency refused: ' + name)
        else:
            missing.append(name)
    if not missing:
        return len(files)
    if destination.exists():
        if any(p.is_symlink() or (p.is_file() and p.relative_to(destination).as_posix() not in files)
               for p in destination.rglob('*')):
            raise ValueError('Unlisted local dependency file refused')
    if digest(archive.read_bytes()) != manifest['archive_sha256']:
        raise ValueError('Assimp archive SHA256 mismatch')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='assimp-stage-', dir=destination.parent) as temp:
        stage = Path(temp) / 'source'
        stage.mkdir()
        with tarfile.open(archive) as package:
            for name, expected in files.items():
                if name in manifest['overlays']:
                    data = (overlay / name).read_bytes()
                else:
                    member = package.getmember(manifest['archive_prefix'] + name)
                    if not member.isfile():
                        raise ValueError('Non-regular archive member refused')
                    data = package.extractfile(member).read()
                if digest(data) != expected:
                    raise ValueError('Dependency file SHA256 mismatch: ' + name)
                path = stage / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
        backup = Path(temp) / 'previous'
        had_destination = destination.exists()
        if had_destination:
            os.replace(destination, backup)
        try:
            os.replace(stage, destination)
        except BaseException:
            if had_destination:
                os.replace(backup, destination)
            raise
    return len(files)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--archive', type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    manifest = json.loads((root / 'tools/assimp_source_manifest.json').read_text())
    destination = root / 'third_party/assimp'
    # A normal full source package needs neither a download nor replacement.
    complete = all((destination/n).is_file() and not (destination/n).is_symlink()
                   and digest((destination/n).read_bytes()) == h for n, h in manifest['files'].items())
    archive = args.archive or root / 'out/dependencies/assimp-v5.4.3.tar.gz'
    if not complete and not archive.exists():
        if args.archive:
            raise FileNotFoundError(archive)
        archive.parent.mkdir(parents=True, exist_ok=True)
        temporary = archive.with_suffix('.download')
        try:
            with urllib.request.urlopen(manifest['archive_url'], timeout=60) as response, temporary.open('wb') as out:
                shutil.copyfileobj(response, out)
            if digest(temporary.read_bytes()) != manifest['archive_sha256']:
                raise ValueError('Downloaded Assimp archive SHA256 mismatch')
            os.replace(temporary, archive)
        finally:
            temporary.unlink(missing_ok=True)
    count = restore(manifest, root / 'tools/assimp_overlay', destination, archive)
    print(f'Verified exact bundled Assimp subset: {count} file hashes; no dependency upgrade.')


if __name__ == '__main__':
    main()
