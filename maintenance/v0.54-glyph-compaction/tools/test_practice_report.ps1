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
    'PASS two attempts, time boundaries, missing samples, camera filter, missing memory, missing CSV and no touchdown'
} finally {Remove-Item $folder -Recurse -Force}
