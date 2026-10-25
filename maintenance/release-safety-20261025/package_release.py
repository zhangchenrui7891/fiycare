"""Create a source-only, deterministic ZIP with safe legacy timestamps."""
import argparse
import hashlib
import json
import os
import tempfile
from pathlib import Path
import zipfile


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    excluded = {'out', '.git', '__pycache__', '.pytest_cache'}
    files = sorted(p for p in root.rglob('*') if p.is_file() and not p.is_symlink()
                   and not excluded.intersection(p.relative_to(root).parts)
                   and p.resolve() != args.output.resolve()
                   and not (p.parent.resolve() == args.output.parent.resolve()
                            and p.name.startswith('.' + args.output.name + '.')
                            and p.name.endswith('.tmp'))
                   and p.name != 'RELEASE_SHA256.json')
    contents = {p.relative_to(root).as_posix(): p.read_bytes() for p in files}
    hashes = {name: hashlib.sha256(data).hexdigest() for name, data in contents.items()}
    contents['RELEASE_SHA256.json'] = (json.dumps(hashes, indent=2) + '\n').encode()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(prefix='.' + args.output.name + '.', suffix='.tmp',
                                         dir=args.output.parent, delete=False) as staging:
            temporary = Path(staging.name)
        with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
            for name, data in sorted(contents.items()):
                info = zipfile.ZipInfo(name, (2000, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o100644 << 16
                archive.writestr(info, data)
        with zipfile.ZipFile(temporary) as archive:
            if archive.testzip() is not None:
                raise ValueError('Release ZIP CRC verification failed')
            for name, digest in hashes.items():
                if hashlib.sha256(archive.read(name)).hexdigest() != digest:
                    raise ValueError('Release ZIP hash verification failed: ' + name)
            if not all(item.date_time == (2000, 1, 1, 0, 0, 0) for item in archive.infolist()):
                raise ValueError('Release ZIP timestamp verification failed')
        os.replace(temporary, args.output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    print(f'{args.output.resolve()}: {len(contents)} files, {args.output.stat().st_size} bytes; CRC, hashes, timestamps verified')


if __name__ == '__main__':
    main()
