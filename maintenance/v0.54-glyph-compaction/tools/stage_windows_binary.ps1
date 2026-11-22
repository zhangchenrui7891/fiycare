param([string]$BuildDirectory = 'out/build/windows-source-check', [string]$Destination = 'out/windows-compat')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
Set-Location $project
$exe = (Resolve-Path (Join-Path $BuildDirectory 'Release/FlightCareerAir.exe')).Path
$cache = Get-Content (Join-Path $BuildDirectory 'CMakeCache.txt') -Raw
if ($cache -notmatch 'CMAKE_MSVC_RUNTIME_LIBRARY:UNINITIALIZED=MultiThreaded' -and $cache -notmatch 'CMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreaded') { throw 'Static runtime configuration missing' }
if ($cache -notmatch 'FCAIR_WITH_ASSIMP:BOOL=OFF') { throw 'Compatibility build must declare importer OFF' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dumpbin = Get-ChildItem (Join-Path $vs 'VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe') | Sort-Object FullName -Descending | Select-Object -First 1
if (!$dumpbin) { throw 'dumpbin.exe not found in Visual Studio C++ tools' }
$dependencies = & $dumpbin.FullName /dependents $exe
if ($LASTEXITCODE -ne 0) { $dependencies | Write-Host; throw 'PE dependency inspection failed' }
if (($dependencies -join "`n") -match '(?i)(MSVCP\d|VCRUNTIME\d|CONCRT\d|ucrtbase\.dll)') { throw 'Dynamic MSVC/UCRT runtime dependency found' }
New-Item -ItemType Directory -Force $Destination | Out-Null
Copy-Item $exe (Join-Path $Destination 'FlightCareerAir.exe')
$dependencies | Set-Content -Encoding utf8 (Join-Path $Destination 'dependencies.txt')
[ordered]@{
    version = '0.103'; source_commit = $env:GITHUB_SHA; configuration = 'Release'; architecture = 'x64';
    compiler = 'MSVC Windows-2022 runner'; msvc_runtime = 'MultiThreaded static'; assimp_importer = $false;
    sha256 = (Get-FileHash -Algorithm SHA256 $exe).Hash.ToLowerInvariant();
    runtime_assets_in_ci = $false; game_executed = $false; gpu_validated = $false; target_hardware_validated = $false
} | ConvertTo-Json | Set-Content -Encoding utf8 (Join-Path $Destination 'build_info.json')
Write-Host 'Staged Windows x64 compatibility EXE and hash/provenance. No game/GPU execution performed.'
