"""Run with Python 3; isolates packaging in a tiny temporary project."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import shutil
import sys
import tempfile
from unittest.mock import patch
import zipfile

with tempfile.TemporaryDirectory() as d:
    root = Path(d)
    (root / 'tools').mkdir()
    script = root / 'tools/package_release.py'
    shutil.copyfile(Path(__file__).with_name('package_release.py'), script)
    (root / 'src').mkdir()
    (root / 'src/main.cpp').write_text('// fixture only\n')
    output = root / 'release.zip'
    spec = importlib.util.spec_from_file_location('release_under_test', script)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    def run():
        with patch.object(sys, 'argv', [str(script), str(output)]), contextlib.redirect_stdout(io.StringIO()):
            mod.main()
    def no_temps():
        assert not list(root.glob('.release.zip.*.tmp'))
    def failure(mock, error):
        before = output.read_bytes() if output.exists() else None
        try:
            with mock:
                run()
        except error:
            pass
        else:
            raise AssertionError('Expected failure was suppressed')
        assert (output.read_bytes() if output.exists() else None) == before
        no_temps()
    # Failed first write must not create a final target.
    failure(patch.object(zipfile.ZipFile, 'writestr', side_effect=OSError('injected write')), OSError)
    run()
    with zipfile.ZipFile(output) as z:
        assert z.testzip() is None
        manifest = json.loads(z.read('RELEASE_SHA256.json'))
        assert 'release.zip' not in manifest
        for n, h in manifest.items():
            assert hashlib.sha256(z.read(n)).hexdigest() == h
    stale = root / '.release.zip.interrupted.tmp'
    stale.write_bytes(b'left by hard termination')
    run()
    with zipfile.ZipFile(output) as z:
        assert stale.name not in z.namelist()
    stale.unlink()
    stable = output.read_bytes()
    run()
    assert output.read_bytes() == stable
    failure(patch.object(zipfile.ZipFile, 'writestr', side_effect=OSError('injected write')), OSError)
    failure(patch.object(zipfile.ZipFile, 'testzip', return_value='src/main.cpp'), ValueError)
    original_read = zipfile.ZipFile.read
    def corrupt_read(self, name, *a, **kw):
        data = original_read(self, name, *a, **kw)
        return data + b'x' if name == 'src/main.cpp' else data
    failure(patch.object(zipfile.ZipFile, 'read', corrupt_read), ValueError)
    failure(patch.object(mod.os, 'replace', side_effect=PermissionError('injected replace')), PermissionError)
    # KeyboardInterrupt models catchable interruption; hard termination is outside this test.
    failure(patch.object(zipfile.ZipFile, 'writestr', side_effect=KeyboardInterrupt()), KeyboardInterrupt)
    (root / 'src/main.cpp').write_text('// replacement fixture\n')
    run()
    assert output.read_bytes() != stable
    with zipfile.ZipFile(output) as z:
        assert z.read('src/main.cpp') == b'// replacement fixture\n'
    no_temps()
print('PASS: 10 scenarios: stale temporary exclusion, failed first write, valid ZIP/hash manifest, deterministic rerun, failed overwrite, CRC rejection, hash rejection, replace denial, interruption, successful replacement')
