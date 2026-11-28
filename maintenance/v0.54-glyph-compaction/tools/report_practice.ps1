[CmdletBinding()]
param([Parameter(Position=0)][string]$Session, [switch]$Rollout)
$ErrorActionPreference = 'Stop'
$invariant = [Globalization.CultureInfo]::InvariantCulture

function Read-Number([string]$Value) {
    $number = 0.0
    if ([double]::TryParse($Value, [Globalization.NumberStyles]::Float, $invariant, [ref]$number) -and
        ![double]::IsNaN($number) -and ![double]::IsInfinity($number)) { return $number }
}
$scenario = if ($Rollout) { 'rollout' } else { 'final' }
$eventKind = if ($Rollout) { 'rollout stopped' } else { 'touchdown' }
$phase = if ($Rollout) { '停稳' } else { '接地' }
if (!$Session) {
    $root = Join-Path $env:LOCALAPPDATA "FlightCareerAir\practice\a330-$scenario\sessions"
    if (Test-Path -LiteralPath $root) {
        $last = Get-ChildItem -LiteralPath $root -Directory | Where-Object {
            Test-Path -LiteralPath (Join-Path $_.FullName 'startup.log')
        } | Sort-Object CreationTimeUtc,Name -Descending | Select-Object -First 1
        if ($last) { $Session = $last.FullName }
    }
}
if (!$Session) { Write-Output "尚无练习记录。请先运行 run_a330_${scenario}.bat。"; return }
$log = Join-Path $Session 'startup.log'
if (!(Test-Path -LiteralPath $log -PathType Leaf)) { throw "找不到练习日志 $log" }
Write-Output "练习目录 $Session"
$events = @(foreach ($line in Get-Content -LiteralPath $log -Encoding UTF8) {
    if ($line -match '^Practice (touchdown|rollout stopped|restart): (.*)$') {
        $kind = $Matches[1]
        if ($kind -ne $eventKind -and $kind -ne 'restart') { continue }
        $values = @{}
        foreach ($pair in [regex]::Matches($Matches[2], '(?:^| )([a-z_]+)=([^ ]+)')) {
            $values[$pair.Groups[1].Value] = $pair.Groups[2].Value
        }
        $time = Read-Number $values.elapsed_s
        if ($null -ne $time -or $kind -eq 'rollout stopped') { [pscustomobject]@{Kind=$kind; Time=$time; Fields=$values} }
    }
})
if (!($events | Where-Object Kind -eq $eventKind)) { Write-Output "尚无${phase}记录。"; return }
$csv = Join-Path $Session 'performance.csv'
$frames = @(if (Test-Path -LiteralPath $csv -PathType Leaf) {
    foreach ($row in Import-Csv -LiteralPath $csv) {
        $time = Read-Number $row.elapsed_s
        if ($null -ne $time -and $row.active -eq '1' -and $row.camera -eq '1' -and
            $row.aircraft -eq '2' -and (Read-Number $row.samples) -gt 0 -and
            (Read-Number $row.p95_ms) -gt 0 -and (Read-Number $row.fps) -gt 0) {
            [pscustomobject]@{Time=$time; Row=$row}
        }
    }
})
$start = 0.0
for ($i=0; $i -lt $events.Count; $i++) {
    $event = $events[$i]
    if ($event.Kind -eq 'restart') { $start = $event.Time; continue }
    $end = [double]::PositiveInfinity
    for ($j=$i+1; $j -lt $events.Count; $j++) {
        if ($events[$j].Kind -eq 'restart') { $end=$events[$j].Time; break }
    }
    $f = $event.Fields
    if ($Rollout) {
        $metrics = foreach ($item in @(
            @{Key='sim_s';Label='用时';Unit='s'}, @{Key='forward_m';Label='沿跑道前进';Unit='m'},
            @{Key='brake_s';Label='刹车';Unit='s'}, @{Key='max_centerline_m';Label='最大偏线';Unit='m'}
        )) {
            $raw = $f[$item.Key]
            if ($null -ne (Read-Number $raw)) { "$($item.Label) $raw $($item.Unit)" }
            else { "$($item.Label) 未记录" }
        }
        Write-Output "第 $($f.attempt) 次滑跑，$($metrics -join '，')"
        if ($null -eq $event.Time) { Write-Output '  缺少停稳的进程时间，无法匹配帧采样。'; continue }
    } else {
        $grade = if ($f.scored -eq '1') { "$($f.score) 分，距中心线 $($f.centerline_m) m" } else { '未评分' }
        $bytes = Read-Number $f.working_set_bytes
        $memory = if ($null -ne $bytes -and $bytes -gt 0) { '{0:F1} MiB' -f ($bytes / 1MB) } else { '未知' }
        Write-Output "第 $($f.attempt) 次练习，接地 $($f.elapsed_s) s，垂直速度 $($f.vy_mps) m/s，$grade，进程工作集 $memory"
    }
    foreach ($side in '前','后') {
        $candidate = if ($side -eq '前') {
            $frames | Where-Object { $_.Time -ge $start -and $_.Time -le $event.Time -and $_.Time -ge $event.Time-6 } | Select-Object -Last 1
        } else {
            $frames | Where-Object { $_.Time -gt $event.Time -and $_.Time -lt $end -and $_.Time -le $event.Time+6 } | Select-Object -First 1
        }
        if (!$candidate) { Write-Output "  ${phase}${side}：六秒内无有效座舱采样"; continue }
        $r = $candidate.Row
        Write-Output "  ${phase}${side}：$($r.elapsed_s) s，$($r.fps) FPS，P95 $($r.p95_ms) ms，$($r.samples) 帧，$($r.backend) $($r.render_width)×$($r.render_height)，FXAA=$($r.fxaa)，限帧=$($r.software_frame_cap)"
    }
}
Write-Output 'P95 来自截至该行时间的滚动窗口。'
if (!$Rollout) { Write-Output '工作集为接地时的进程内存。' }
