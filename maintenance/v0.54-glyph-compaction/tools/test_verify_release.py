"""Test the read-only release verifier using isolated, tiny source trees."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--cmake', required=True)
args = parser.parse_args()
verifier = Path(__file__).with_name('verify_release.cmake')
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory) / '工程 with spaces'
    root.mkdir()
    source = root / 'main.cpp'
    source.write_bytes(b'// release fixture\n')
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    manifest = root / 'RELEASE_SHA256.json'
    cases = [
        ('valid', {'main.cpp': digest}, True),
        ('missing', {'absent.cpp': digest}, False),
        ('corrupt', {'main.cpp': '0' * 64}, False),
        ('bad digest', {'main.cpp': 'xyz'}, False),
        ('empty', {}, False),
        ('wrong type', [], False),
        ('traversal', {'../main.cpp': digest}, False),
        ('absolute', {'/main.cpp': digest}, False),
        ('backslash', {'dir\\main.cpp': digest}, False),
        ('non-string digest', {'main.cpp': 123}, False),
        ('directory', {'.': digest}, False),
    ]
    for label, data, success in cases:
        manifest.write_text(json.dumps(data))
        result = subprocess.run([args.cmake, '-DFCAIR_ROOT=' + str(root), '-P', str(verifier)],
                                text=True, capture_output=True)
        if (result.returncode == 0) != success:
            raise RuntimeError(label + '\n' + result.stdout + result.stderr)
        if source.read_bytes() != b'// release fixture\n':
            raise RuntimeError('Verifier modified source')
    manifest.write_text('{broken-json')
    result = subprocess.run([args.cmake, '-DFCAIR_ROOT=' + str(root), '-P', str(verifier)], capture_output=True)
    if result.returncode == 0:
        raise RuntimeError('Malformed JSON accepted')
print('PASS: 12 release verifier scenarios; read-only check, valid/changed/missing/invalid manifest files.')
