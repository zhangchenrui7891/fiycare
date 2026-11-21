$ErrorActionPreference = 'Stop'
$fixture = Join-Path ([IO.Path]::GetTempPath()) ('fcair delivery ' + [guid]::NewGuid().ToString('N'))
try {
    $helper = Join-Path $fixture 'tools'
    New-Item -ItemType Directory -Force $helper | Out-Null
    Copy-Item (Join-Path $PSScriptRoot 'resolve_flight_exe.bat') $helper
    $driver = Join-Path $fixture 'probe.bat'
    @'
@echo off
call "%~dp0tools\resolve_flight_exe.bat"
if errorlevel 1 exit /b 7
if not exist "%FCAIR_APP%" exit /b 8
>"%~dp0selected.txt" echo %FCAIR_APP%
'@ | Set-Content -Encoding ascii $driver
    function Probe([int]$expectedExit, [string]$expectedPath) {
        & $env:ComSpec /d /c "`"$driver`""
        if ($LASTEXITCODE -ne $expectedExit) { throw "Resolver exit: expected $expectedExit got $LASTEXITCODE" }
        if ($expectedExit -eq 0) {
            $actual = (Get-Content (Join-Path $fixture 'selected.txt') -Raw).Trim()
            if ([IO.Path]::GetFullPath($actual) -ne [IO.Path]::GetFullPath($expectedPath)) { throw 'Wrong executable selected' }
        }
    }
    Probe 7 ''
    $packaged = Join-Path $fixture 'FlightCareerAir.exe'
    Set-Content -Encoding ascii $packaged 'fixture, not executable'
    Probe 0 $packaged
    $rebuilt = Join-Path $fixture 'out/build/release/FlightCareerAir.exe'
    New-Item -ItemType Directory -Force (Split-Path $rebuilt) | Out-Null
    Set-Content -Encoding ascii $rebuilt 'fixture, not executable'
    Probe 0 $rebuilt
    Remove-Item $rebuilt
    Probe 0 $packaged
    Write-Host 'PASS actual cmd resolver: missing, packaged fallback, rebuilt priority, removal fallback; paths with spaces. No game process was launched.'
} finally {
    Remove-Item -Recurse -Force $fixture -ErrorAction SilentlyContinue
}
