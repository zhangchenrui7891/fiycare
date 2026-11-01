"""Exercise the production staging function with actual incremental CMake builds.

The fixture executable is deliberately tiny; this does not launch the simulator.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(args, ok=True):
    p = subprocess.run(args, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if (p.returncode == 0) != ok:
        raise RuntimeError(f'{args}: exit {p.returncode}\n{p.stdout}')
    return p.stdout


def check(cmake, generator, staging_helper=None):
    with tempfile.TemporaryDirectory(prefix='fcair assets 中文 ') as temporary:
        root = Path(temporary)
        source = root / 'source tree'
        source.mkdir()
        assets = source / 'assets'
        (assets / '机模').mkdir(parents=True)
        asset = assets / '机模/test.fcmesh'
        asset.write_bytes(b'original-model\0')
        (source / 'main.cpp').write_text('int main() { return 0; }\n')
        helper = (Path(staging_helper) if staging_helper else Path(__file__).with_name('runtime_assets.cmake')).resolve().as_posix()
        (source / 'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.22)
project(StagingFixture LANGUAGES CXX)
option(FCAIR_COPY_RUNTIME_ASSETS "Stage assets" ON)
add_executable(fixture main.cpp)
set_target_properties(fixture PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
include("''' + helper + '''")
fcair_stage_runtime_assets(fixture "${CMAKE_CURRENT_SOURCE_DIR}/assets")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/exe-$<CONFIG>.txt" CONTENT "$<TARGET_FILE:fixture>")
''', encoding='utf-8')
        build = root / 'build tree'
        run([cmake, '-S', str(source), '-B', str(build), '-G', generator,
             '-DCMAKE_BUILD_TYPE=Release'])

        def compile_fixture(ok=True):
            return run([cmake, '--build', str(build), '--config', 'Release',
                        '--target', 'fixture', '--parallel', '2'], ok)

        compile_fixture()
        exe = Path((build / 'exe-Release.txt').read_text())
        output = exe.parent / 'assets/机模/test.fcmesh'
        assert output.read_bytes() == asset.read_bytes()
        stamp = (exe.stat().st_mtime_ns, hashlib.sha256(exe.read_bytes()).hexdigest())
        asset.write_bytes(b'new-scenery-model\0')
        compile_fixture()
        assert output.read_bytes() == asset.read_bytes(), 'resource-only edit was not staged'
        assert stamp == (exe.stat().st_mtime_ns, hashlib.sha256(exe.read_bytes()).hexdigest()), 'unexpected relink'
        shutil.rmtree(exe.parent / 'assets')
        compile_fixture()
        assert output.read_bytes() == asset.read_bytes(), 'missing output was not repaired'
        shutil.rmtree(assets)
        compile_fixture(ok=False)
        (assets / '机模').mkdir(parents=True)
        asset.write_bytes(b'restored-source')
        compile_fixture()
        assert output.read_bytes() == b'restored-source', 'failed stage could not be retried'
        run([cmake, '-S', str(source), '-B', str(build), '-DFCAIR_COPY_RUNTIME_ASSETS=OFF'])
        shutil.rmtree(exe.parent / 'assets')
        compile_fixture()
        assert not (exe.parent / 'assets').exists(), 'OFF copied assets'
        run([cmake, '-S', str(source), '-B', str(build), '-DFCAIR_COPY_RUNTIME_ASSETS=ON'])
        compile_fixture()
        assert output.read_bytes() == asset.read_bytes(), 'ON did not restore assets'
        print(f'{generator}: initial, resource-only/no-relink, missing output, failure, retry, OFF and ON passed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--generator', action='append')
    parser.add_argument('--helper')
    args = parser.parse_args()
    for name in args.generator or (['Visual Studio 17 2022'] if os.name == 'nt' else
                                   ['Ninja', 'Ninja Multi-Config'] if shutil.which('ninja') else
                                   ['Unix Makefiles']):
        check(args.cmake, name, args.helper)
