"""Exercise the real CMake repair and Ninja regeneration, without Windows claims."""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cmake', default=shutil.which('cmake'))
    parser.add_argument('--ninja', default=shutil.which('ninja'))
    args = parser.parse_args()
    if not args.cmake or not args.ninja:
        parser.error('Both CMake and Ninja are required for the regression')
    source = Path(__file__).resolve().parent / 'repair_build_timestamps.cmake'

    def run(command, ok=True):
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=60)
        if (result.returncode == 0) != ok:
            raise RuntimeError(result.stdout)
        return result.stdout

    with tempfile.TemporaryDirectory(prefix='fcair timestamp space ') as directory:
        root = Path(directory)
        build = root / 'out'
        (root / 'tools').mkdir()
        (root / 'third_party/fake').mkdir(parents=True)
        shutil.copyfile(source, root / 'tools/repair_build_timestamps.cmake')
        dependency = root / 'third_party/fake/policy.cmake'
        dependency.write_text('# Build input from an extracted archive\n')
        main_file = root / 'CMakeLists.txt'
        main_file.write_text('cmake_minimum_required(VERSION 3.22)\n'
                             'project(Reproduction LANGUAGES NONE)\n'
                             'include(third_party/fake/policy.cmake)\n')
        configure = [args.cmake, '-S', str(root), '-B', str(build), '-G', 'Ninja',
                     '-DCMAKE_MAKE_PROGRAM=' + args.ninja]
        run(configure)
        future = time.time() + 8 * 3600
        os.utime(dependency, (future, future))
        # Reproduce the exact fatal error in a minimal old-style build.
        old_output = run([args.ninja, '-C', str(build), '-d', 'explain'], ok=False)
        assert 'still dirty after 100 tries' in old_output, old_output
        assert 'older than most recent input' in old_output, old_output
        print('PASS: future CMake input reproduces Ninja 100-try failure')

        main_file.write_text('cmake_minimum_required(VERSION 3.22)\n'
                             'include(tools/repair_build_timestamps.cmake)\n'
                             'fcair_check_build_timestamps("${CMAKE_CURRENT_LIST_DIR}" FALSE)\n'
                             'project(Repaired LANGUAGES NONE)\n'
                             'include(third_party/fake/policy.cmake)\n')
        error = run(configure, ok=False)
        assert 'FCAIR_FUTURE_TIMESTAMPS' in error, error
        assert 'repair_build.bat' in error, error
        print('PASS: configuration stops early with actionable diagnosis')

        cpp = root / 'future.cpp'
        cpp.write_text('int main() { return 0; }\n')
        os.utime(cpp, (future, future))
        old = root / 'old.cpp'
        old.write_text('// Existing, correctly dated source\n')
        past = time.time() - 3600
        os.utime(old, (past, past))
        ignored = build / 'generated.cmake'
        ignored.write_text('# Generated output must not be touched\n')
        os.utime(ignored, (future, future))
        files = [dependency, cpp, old, ignored]
        hashes = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
        mtimes = {p: p.stat().st_mtime_ns for p in files}
        repair = [args.cmake, '-DFCAIR_ROOT=' + str(root), '-DFCAIR_REPAIR=ON',
                  '-P', str(root / 'tools/repair_build_timestamps.cmake')]
        message = run(repair)
        assert 'normalized 2 future' in message, message
        for path in files:
            assert hashlib.sha256(path.read_bytes()).hexdigest() == hashes[path]
        for path in [old, ignored]:
            assert path.stat().st_mtime_ns == mtimes[path]
        assert dependency.stat().st_mtime <= time.time()
        assert cpp.stat().st_mtime <= time.time()
        assert 'normalized 0 future' in run(repair)
        print('PASS: only future source/input mtimes change; contents and outputs preserved; repeat is idempotent')

        run(configure)
        for _ in range(2):
            result = run([args.ninja, '-C', str(build), '-n'])
            assert 'no work to do' in result and 'Re-running CMake' not in result, result
        print('PASS: two clean incremental Ninja builds after repair')
        main_file.write_text(main_file.read_text() + '# A legitimate configuration edit\n')
        result = run([args.ninja, '-C', str(build)])
        assert 'Re-running CMake' in result and 'still dirty' not in result, result
        print('PASS: real CMake edits still regenerate normally')

        # Validate the actual vendor command lines under strict CMP0175 rules.
        # This tests CMake grammar on Linux; it does not compile MSVC or run DLL copies.
        version = run([args.cmake, '--version']).splitlines()[0].split()[-1]
        if tuple(int(part) for part in version.split('.')[:2]) >= (3, 31):
            vendor = source.parents[1] / 'third_party/assimp/CMakeLists.txt'
            commands = [line.strip() for line in vendor.read_text().splitlines()
                        if 'ADD_CUSTOM_COMMAND(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs' in line]
            assert commands and all(' POST_BUILD COMMAND ' in line for line in commands)
            fixture = root / 'vendor'
            fixture.mkdir()
            header = ('cmake_minimum_required(VERSION 3.31)\n'
                      'project(VendorCommands LANGUAGES NONE)\n'
                      'add_custom_target(UpdateAssimpLibsDebugSymbolsAndDLLs)\n')
            script = fixture / 'CMakeLists.txt'
            vendor_configure = [args.cmake, '-S', str(fixture), '-B', str(root / 'vendor-out'),
                                '-G', 'Ninja', '-DCMAKE_MAKE_PROGRAM=' + args.ninja]
            script.write_text(header + '\n'.join(line.replace(' POST_BUILD ', ' ') for line in commands))
            old_error = run(vendor_configure, ok=False)
            assert 'PRE_BUILD' in old_error or 'POST_BUILD' in old_error, old_error
            script.write_text(header + '\n'.join(commands))
            run(vendor_configure)
            print(f'PASS: {len(commands)} actual Assimp command lines pass strict CMP0175 grammar (old forms rejected)')
        else:
            print('SKIP: strict CMP0175 grammar check requires CMake 3.31+')


if __name__ == '__main__':
    main()
