$ErrorActionPreference = 'Stop'
$root = Join-Path $env:TEMP ('fcair launcher ' + [guid]::NewGuid().ToString('N'))
$project = Split-Path -Parent $PSScriptRoot
$oldProfile = $env:LOCALAPPDATA
$oldLog = $env:FCAIR_PROBE_LOG
try {
    $tools = Join-Path $root 'tools'
    New-Item -ItemType Directory -Force $tools | Out-Null
    Copy-Item (Join-Path $PSScriptRoot 'resolve_flight_exe.bat') $tools
    $env:LOCALAPPDATA = Join-Path $root 'profile'
    $env:FCAIR_PROBE_LOG = Join-Path $root 'calls.txt'
    $probe = Join-Path $root 'FlightCareerAir.exe'
    Add-Type -OutputType ConsoleApplication -OutputAssembly $probe -TypeDefinition @'
using System;
using System.IO;
using System.Reflection;
class Probe {
    static void Main(string[] args) {
        File.AppendAllText(Environment.GetEnvironmentVariable("FCAIR_PROBE_LOG"),
            Assembly.GetExecutingAssembly().Location + "|" + String.Join(" ",args) + Environment.NewLine);
        if (Array.IndexOf(args,"--benchmark-777-cockpit") >= 0) {
            string folder = Path.Combine(Environment.GetEnvironmentVariable("LOCALAPPDATA"),
                "FlightCareerAir","diagnostics","benchmark",
                Array.IndexOf(args,"--no-aa") >= 0 ? "no-fxaa" : "", "777-cockpit");
            Directory.CreateDirectory(folder);
            File.WriteAllText(Path.Combine(folder,"performance_summary.txt"), "result_status=VALID\r\n");
        }
    }
}
'@
    foreach ($variant in @('release','cockpit')) {
        $dir = Join-Path $root "out/build/$variant"
        New-Item -ItemType Directory -Force $dir | Out-Null
        Copy-Item $probe $dir
    }
    $expectedExe = Join-Path $root 'out\build\cockpit\FlightCareerAir.exe'
    foreach ($script in @('run_compat.bat','run_benchmark_dx11.bat','run_benchmark_777_fxaa_pair.bat')) {
        $bat = Join-Path $root $script
        Copy-Item (Join-Path $project $script) $bat
        & $env:ComSpec /d /c "call `"$bat`" <nul"
        if ($script -eq 'run_benchmark_777_fxaa_pair.bat' -and $LASTEXITCODE -ne 0) { throw 'Paired benchmark launcher failed' }
        $deadline = [DateTime]::UtcNow.AddSeconds(10)
        while (!(Test-Path $env:FCAIR_PROBE_LOG) -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 100 }
    }
    $expectedArgs = @('--dx11 --low','--dx11 --low --benchmark','--dx11 --benchmark --benchmark-777-cockpit','--dx11 --benchmark --benchmark-777-cockpit --no-aa')
    $calls = @(Get-Content $env:FCAIR_PROBE_LOG)
    if ($calls.Count -ne $expectedArgs.Count) { throw "Expected four process launches, got $($calls.Count)" }
    for ($i=0; $i -lt $calls.Count; $i++) {
        if ($calls[$i] -ne "$expectedExe|$($expectedArgs[$i])") { throw "Unexpected executable or arguments: $($calls[$i])" }
    }
    Write-Host 'PASS: three production batch entry points launched four cockpit probe processes with the expected flags. No simulator performance was measured.'
} finally {
    $env:LOCALAPPDATA = $oldProfile
    $env:FCAIR_PROBE_LOG = $oldLog
    Remove-Item -Recurse -Force $root -ErrorAction SilentlyContinue
}
