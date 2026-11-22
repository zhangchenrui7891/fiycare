param([string]$Executable = 'out/windows-compat/FlightCareerAir.exe', [string]$OutputDirectory = 'out/windows-smoke')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
Set-Location $project
$sourceExe = (Resolve-Path $Executable).Path
$root = if ([IO.Path]::IsPathRooted($OutputDirectory)) { [IO.Path]::GetFullPath($OutputDirectory) } else { [IO.Path]::GetFullPath((Join-Path $project $OutputDirectory)) }
$runtime = Join-Path $root 'fixture-runtime'
$evidence = Join-Path $root 'evidence'
$asset = Join-Path $runtime 'assets'
New-Item -ItemType Directory -Force $asset, $evidence | Out-Null
$app = Join-Path $runtime 'FlightCareerAir.exe'
Copy-Item $sourceExe $app -Force
$atlas = Join-Path $asset 'atlas.fctex'
$writer = [IO.BinaryWriter]::new([IO.File]::Create($atlas))
try {
    $writer.Write([Text.Encoding]::ASCII.GetBytes('FCTEX005'))
    $writer.Write([uint32]64); $writer.Write([uint32]64)
    for ($y=0; $y -lt 64; $y++) { for ($x=0; $x -lt 64; $x++) {
        $bright = (([int][Math]::Floor($x/8) + [int][Math]::Floor($y/8)) % 2) -eq 0
        $writer.Write([byte]$(if ($bright) { 80 } else { 35 }))
        $writer.Write([byte]$(if ($bright) { 140 } else { 70 }))
        $writer.Write([byte]$(if ($bright) { 75 } else { 45 }))
        $writer.Write([byte]255)
    } }
} finally { $writer.Dispose() }
'CI FIXTURE ONLY: generated 64x64 checker atlas, procedural trainer, Windows DX11 WARP software rendering. Not full released assets, target GPU, FPS or full-flight evidence.' | Set-Content -Encoding utf8 (Join-Path $evidence 'README.txt')
$oldProfile = $env:LOCALAPPDATA
$cases = @()
try {
    foreach ($camera in @('chase','cockpit','missing-atlas')) {
        $profile = Join-Path $root ('profile-' + $camera)
        $live = Join-Path $profile 'FlightCareerAir'
        New-Item -ItemType Directory -Force $live | Out-Null
        $sentinels = @{}
        foreach ($name in @('state.txt','state.bak','career.txt','tracks.txt')) {
            $file = Join-Path $live $name
            'FCAIR CI sentinel' | Set-Content -Encoding ascii $file
            $sentinels[$file] = (Get-FileHash $file).Hash
        }
        $env:LOCALAPPDATA = $profile
        $arguments = @('--dx11','--warp','--low','--smoke-test','--diagnostic-no-dialog')
        if ($camera -eq 'cockpit') { $arguments += '--cockpit' }
        if ($camera -eq 'missing-atlas') { Move-Item $atlas ($atlas + '.off') -Force }
        $process = Start-Process -FilePath $app -ArgumentList $arguments -WorkingDirectory $runtime -PassThru
        if (!$process.WaitForExit(60000)) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            throw "Smoke fixture timed out: $camera"
        }
        $process.WaitForExit()
        $exitCode = $process.ExitCode
        $folder = Join-Path $live 'diagnostics/smoke'
        $log = Join-Path $folder 'startup.log'
        if (!(Test-Path $log)) { throw "Startup log missing: $camera" }
        Copy-Item $log (Join-Path $evidence ($camera + '-startup.log')) -Force
        $text = Get-Content $log -Raw
        foreach ($file in $sentinels.Keys) {
            if ((Get-FileHash $file).Hash -ne $sentinels[$file]) { throw 'Diagnostic execution changed live save sentinel' }
        }
        if ($camera -eq 'missing-atlas') {
            if ($exitCode -ne 1 -or $text -notmatch 'Required texture atlas.fctex') { throw 'Missing required atlas did not return logged failure' }
            $cases += [ordered]@{case=$camera;exit_code=$exitCode;expected_failure=$true;live_saves_unchanged=$true}
            continue
        }
        if ($exitCode -ne 0) { throw "Smoke fixture returned $exitCode : $camera" }
        if ($text -notmatch 'Selected DX11 software \(diagnostic only\)' -or $text -notmatch 'Smoke test: three rendered frames and BMP capture succeeded' -or $text -notmatch 'Normal shutdown') { throw "Expected DX11 WARP/frame/shutdown evidence missing: $camera" }
        $captures = @(Get-ChildItem $folder -Filter '*.bmp')
        if ($captures.Count -ne 1) { throw "Expected one frame capture: $camera" }
        $bytes = [IO.File]::ReadAllBytes($captures[0].FullName)
        $width = [BitConverter]::ToInt32($bytes,18); $height = [Math]::Abs([BitConverter]::ToInt32($bytes,22))
        $offset = [BitConverter]::ToInt32($bytes,10)
        if ($bytes[0] -ne 66 -or $bytes[1] -ne 77 -or $width -ne 640 -or $height -ne 360 -or [BitConverter]::ToInt16($bytes,28) -ne 32 -or $bytes.Length -ne ($offset+640*360*4)) { throw 'Invalid fixture screenshot format/extent/payload' }
        $colors = [Collections.Generic.HashSet[uint32]]::new()
        for ($i=$offset; $i -lt ($bytes.Length-3); $i+=388) { [void]$colors.Add([BitConverter]::ToUInt32($bytes,$i) -band 0xffffff) }
        if ($colors.Count -lt 16) { throw "Frame capture appears blank: $camera" }
        $capture = Join-Path $evidence ('CI_FIXTURE_WARP_DX11_' + $camera + '.bmp')
        Copy-Item $captures[0].FullName $capture -Force
        $cases += [ordered]@{case=$camera;exit_code=$exitCode;rendered_frames=3;width=$width;height=$height;sampled_colors=$colors.Count;bmp_sha256=(Get-FileHash $capture).Hash.ToLowerInvariant();live_saves_unchanged=$true}
    }
    $comparisonPath = Join-Path $evidence 'notice_layout_comparison.json'
    & python (Join-Path $PSScriptRoot 'check_notice_pixels.py') --baseline (Join-Path $project 'validation_v102/CI_FIXTURE_WARP_EVIDENCE.zip') --current $evidence --output $comparisonPath
    if ($LASTEXITCODE -ne 0) { throw 'Actual fixture notification layout comparison failed' }
    $comparison = Get-Content $comparisonPath -Raw | ConvertFrom-Json
    $summary = [ordered]@{source_commit=$env:GITHUB_SHA;game_executed=$true;environment='Windows CI DX11 WARP software';fixture_atlas='generated 64x64 checker';aircraft='procedural trainer 0';full_assets_validated=$false;hardware_gpu_validated=$false;target_hardware_validated=$false;performance_verdict='NOT_EVALUATED';notice_layout_comparison=$comparison;cases=$cases}
    $summary | ConvertTo-Json -Depth 6 | Set-Content -Encoding utf8 (Join-Path $evidence 'smoke_summary.json')
    $destination = Split-Path $sourceExe
    Copy-Item (Join-Path $evidence 'smoke_summary.json') (Join-Path $destination 'smoke_summary.json') -Force
    $infoPath = Join-Path $destination 'build_info.json'
    $info = Get-Content $infoPath -Raw | ConvertFrom-Json
    Copy-Item $comparisonPath (Join-Path $destination 'notice_layout_comparison.json') -Force
    $info.game_executed = $true
    $info | Add-Member -NotePropertyName cockpit_notice_layout_verified -NotePropertyValue $true -Force
    $info | Add-Member -NotePropertyName game_execution_environment -NotePropertyValue 'Windows CI DX11 WARP; synthetic atlas; procedural trainer' -Force
    $info | Add-Member -NotePropertyName full_assets_validated -NotePropertyValue $false -Force
    $info | ConvertTo-Json -Depth 6 | Set-Content -Encoding utf8 $infoPath
    Write-Host 'PASS actual Windows application: chase/cockpit each rendered three DX11 WARP frames and BMP; missing atlas exited 1 without modal wait; save sentinels unchanged. Synthetic fixture only, no target GPU/FPS/full-flight verdict.'
} finally { $env:LOCALAPPDATA = $oldProfile }
