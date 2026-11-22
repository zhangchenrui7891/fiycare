$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('fcair delivery ' + [guid]::NewGuid().ToString('N'))
try {
    $helper = Join-Path $fixture 'tools'
    New-Item -ItemType Directory -Force $helper | Out-Null
    Copy-Item (Join-Path $PSScriptRoot 'resolve_flight_exe.bat') $helper
    $driver = Join-Path $fixture 'probe.bat'
    @'
@echo off
set "FCAIR_APP=stale"
call "%~dp0tools\resolve_flight_exe.bat" %1
if errorlevel 1 (
  if defined FCAIR_APP exit /b 9
  exit /b 7
)
if not exist "%FCAIR_APP%" exit /b 8
>"%~dp0selected.txt" echo %FCAIR_APP%
'@ | Set-Content -Encoding ascii $driver
    $paths = @(
        (Join-Path $fixture 'FlightCareerAir.exe')
        (Join-Path $fixture 'out/build/release/FlightCareerAir.exe')
        (Join-Path $fixture 'out/build/cockpit/FlightCareerAir.exe')
    )
    foreach ($file in $paths) { New-Item -ItemType Directory -Force (Split-Path $file) | Out-Null }
    foreach ($mask in 0..7) {
        for ($i=0; $i -lt 3; $i++) {
            if ($mask -band (1 -shl $i)) { Set-Content -Encoding ascii $paths[$i] 'resolver fixture' }
            elseif (Test-Path $paths[$i]) { Remove-Item $paths[$i] }
        }
        foreach ($mode in @('','cockpit')) {
            $order = if ($mode) { 2,1,0 } else { 1,2,0 }
            $expected = $order | Where-Object { Test-Path $paths[$_] } | Select-Object -First 1
            & $env:ComSpec /d /c "call `"$driver`" $mode"
            if ($null -eq $expected) {
                if ($LASTEXITCODE -ne 7) { throw "Missing EXE: $LASTEXITCODE" }
            } else {
                if ($LASTEXITCODE -ne 0) { throw "Resolver failed for $mask/$mode" }
                $actual = (Get-Content (Join-Path $fixture 'selected.txt') -Raw).Trim()
                if ([IO.Path]::GetFullPath($actual) -ne $paths[$expected]) { throw "Wrong EXE for $mask/$mode" }
            }
        }
    }
    Write-Host 'PASS: 16 actual cmd resolver cases; three locations, two priorities, stale value cleared, paths with spaces.'
} finally {
    Remove-Item -Recurse -Force $fixture -ErrorAction SilentlyContinue
}
