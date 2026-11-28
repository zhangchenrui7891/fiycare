$ErrorActionPreference='Stop'
$folder=Join-Path ([IO.Path]::GetTempPath()) ([guid]::NewGuid().ToString())
New-Item -Type Directory $folder | Out-Null
$report=Join-Path $PSScriptRoot 'report_practice.ps1'
try {
    Set-Content (Join-Path $folder 'startup.log') @(
        'Practice touchdown: vy_mps=-4.17 attempt=1 elapsed_s=80 sim_s=78 scored=1 score=62 centerline_m=0 working_set_bytes=104857600',
        'Practice restart: attempt=2 elapsed_s=82',
        'Practice touchdown: vy_mps=-9 attempt=2 elapsed_s=84 scored=0 crash=test working_set_bytes=NA')
    Set-Content (Join-Path $folder 'performance.csv') @(
        'elapsed_s,backend,aircraft,camera,terrain,fxaa,low_mode,active,samples,fps,p95_ms,max_ms,render_width,render_height,software_frame_cap',
        '75,DX11,2,1,1,1,1,1,100,20,51,70,640,360,20',
        '79,DX11,2,0,1,1,1,1,100,20,999,999,640,360,20',
        '85,DX11,2,1,1,1,1,1,100,19,53,80,640,360,20',
        '86,DX11,2,1,1,1,1,0,0,0,888,888,640,360,20')
    $s=(& $report $folder) -join "`n"
    foreach($expected in 'P95 51 ms','P95 53 ms','62 分','100.0 MiB','未评分','工作集 未知','接地后：六秒内无有效座舱采样','接地前：六秒内无有效座舱采样') {
        if (!$s.Contains($expected)) {throw "Missing $expected in $s"}
    }
    if($s -match '999|888'){throw 'Incorrect context'}
    Write-Output $s
    $original=$env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA=$folder
        $session=Join-Path $folder 'FlightCareerAir\practice\a330-final\sessions\session with spaces'
        New-Item $session -ItemType Directory -Force | Out-Null
        Copy-Item (Join-Path $folder 'startup.log'),(Join-Path $folder 'performance.csv') $session
        Push-Location (Split-Path $PSScriptRoot)
        try {
            $actual=cmd.exe /d /c 'chcp 65001 >nul && call report_a330_final.bat <nul'
            if ($LASTEXITCODE -ne 0 -or ($actual -join "`n") -notmatch 'P95 51 ms') {throw 'Double-click entry failed'}
        } finally {Pop-Location}
    } finally {$env:LOCALAPPDATA=$original}

    Remove-Item (Join-Path $folder 'performance.csv')
    $s=(& $report $folder) -join "`n"
    if ($s -match ' FPS' -or !$s.Contains('62 分')) {throw 'Missing CSV handling'}
    Clear-Content (Join-Path $folder 'startup.log')
    if ((& $report $folder) -notcontains '尚无接地记录。') {throw 'Empty session handling'}
    $root=Join-Path $folder 'FlightCareerAir\practice\a330-rollout\sessions\rollout with spaces'
    New-Item -ItemType Directory -Force $root | Out-Null
    Set-Content (Join-Path $root 'startup.log') @(
        'Practice rollout stopped: attempt=1 sim_s=16.890 z_m=-951.862 forward_m=448.138 brake_s=16.890 max_centerline_m=0.000 elapsed_s=24.000',
        'Practice restart: attempt=2 elapsed_s=26',
        'Practice rollout stopped: attempt=2 sim_s=18.200 z_m=-876.390 forward_m=523.610 brake_s=16.200 max_centerline_m=0.000 elapsed_s=54.000')
    Set-Content (Join-Path $root 'performance.csv') @(
        'elapsed_s,backend,aircraft,camera,terrain,fxaa,low_mode,active,samples,fps,p95_ms,max_ms,render_width,render_height,software_frame_cap',
        '20,DX11,2,1,1,1,1,1,100,20,51,70,640,360,20',
        '27,DX11,2,1,1,1,1,1,100,19,999,999,640,360,20',
        '55,DX11,2,1,1,1,1,1,100,18,54,80,640,360,20')
    $s=(& $report $root -Rollout) -join "`n"
    foreach($expected in '448.138 m','523.610 m','刹车 16.200 s','停稳前：20 s','停稳后：55 s','停稳后：六秒内无有效座舱采样','停稳前：六秒内无有效座舱采样') {
        if (!$s.Contains($expected)) { throw "Rollout report missing $expected in $s" }
    }
    if ($s -match '999|接地'){throw 'Wrong rollout sample or phase'}
    Write-Output $s
    $original=$env:LOCALAPPDATA
    try {
        $env:LOCALAPPDATA=$folder
        Push-Location (Split-Path $PSScriptRoot)
        try {
            $actual=cmd.exe /d /c 'chcp 65001 >nul && call report_a330_rollout.bat <nul'
            if ($LASTEXITCODE -ne 0 -or ($actual -join "`n") -notmatch '448.138 m') {throw 'Rollout launcher failed'}
        } finally {Pop-Location}
    } finally {$env:LOCALAPPDATA=$original}
    Set-Content (Join-Path $root 'startup.log') 'Practice rollout stopped: attempt=1 sim_s=8.67 z_m=-1267.45'
    $s=(& $report $root -Rollout) -join "`n"
    if (!$s.Contains('用时 8.67 s') -or !$s.Contains('沿跑道前进 未记录') -or $s.Contains(' FPS') -or !$s.Contains('无法匹配帧采样')) {throw 'Legacy rollout report failed'}
    'PASS rollout results, frame pairing, restart isolation, legacy log, and normal launcher'
    'PASS two attempts, time boundaries, missing samples, camera filter, missing memory, missing CSV and no touchdown'
} finally {Remove-Item $folder -Recurse -Force}
