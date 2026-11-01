param([string]$BuildDirectory = 'out/build/windows-check')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
Set-Location $project
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$build = (Resolve-Path $BuildDirectory).Path
Start-Transcript -Path (Join-Path $build 'windows-check.log') -Force | Out-Null
try {
    & cmake -DFCAIR_REPAIR=ON -P tools/repair_build_timestamps.cmake
    if ($LASTEXITCODE -ne 0) { throw 'Timestamp repair failed' }
    Write-Host 'Configure Windows x64 Release (Visual Studio 2022)'
    & cmake -S $project -B $build -G 'Visual Studio 17 2022' -A x64
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
    & cmake --build $build --config Release --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'Windows build failed' }
    & ctest --test-dir $build -C Release --output-on-failure --timeout 180
    if ($LASTEXITCODE -ne 0) { throw 'Logic/resource tests failed' }
    if (!(Test-Path (Join-Path $build 'Release/FlightCareerAir.exe'))) { throw 'Missing application executable' }
    if (!(Test-Path (Join-Path $build 'Release/assets/atlas.fctex'))) { throw 'Missing runtime assets' }
    Write-Host 'BUILD AND LOGIC TESTS PASSED. Graphics, mouse, Chinese text and HD6000 performance are NOT verified.'
} finally {
    Stop-Transcript | Out-Null
}
