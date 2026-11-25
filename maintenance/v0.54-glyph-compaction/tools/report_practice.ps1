[CmdletBinding()]
param([string]$Session)
$ErrorActionPreference = 'Stop'
$invariant = [Globalization.CultureInfo]::InvariantCulture

function Read-Number([string]$Value) {
    $number = 0.0
    if ([double]::TryParse($Value, [Globalization.NumberStyles]::Float, $invariant, [ref]$number) -and
        ![double]::IsNaN($number) -and ![double]::IsInfinity($number)) { return $number }
}
if (!$Session) {
    $root = Join-Path $env:LOCALAPPDATA 'FlightCareerAir\practice\a330-final\sessions'
    if (Test-Path -LiteralPath $root) {
        $last = Get-ChildItem -LiteralPath $root -Directory | Where-Object {
            Test-Path -LiteralPath (Join-Path $_.FullName 'startup.log')
        } | Sort-Object CreationTimeUtc,Name -Descending | Select-Object -First 1
        if ($last) { $Session = $last.FullName }
    }
}
if (!$Session) { Write-Output '尚无练习记录。请先运行 run_a330_final.bat。'; return }
$log = Join-Path $Session 'startup.log'
if (!(Test-Path -LiteralPath $log -PathType Leaf)) { throw "找不到练习日志 $log" }
Write-Output "练习目录 $Session"
$events = @(foreach ($line in Get-Content -LiteralPath $log -Encoding UTF8) {
    if ($line -match '^Practice (touchdown|restart): (.*)$') {
        $kind = $Matches[1]
        $values = @{}
        foreach ($pair in [regex]::Matches($Matches[2], '(?:^| )([a-z_]+)=([^ ]+)')) {
            $values[$pair.Groups[1].Value] = $pair.Groups[2].Value
        }
        $time = Read-Number $values.elapsed_s
        if ($null -ne $time) { [pscustomobject]@{Kind=$kind; Time=$time; Fields=$values} }
    }
})
if (!($events | Where-Object Kind -eq 'touchdown')) { Write-Output '尚无接地记录。'; return }
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
    $grade = if ($f.scored -eq '1') { "$($f.score) 分，距中心线 $($f.centerline_m) m" } else { '未评分' }
    $bytes = Read-Number $f.working_set_bytes
    $memory = if ($null -ne $bytes -and $bytes -gt 0) { '{0:F1} MiB' -f ($bytes / 1MB) } else { '未知' }
    Write-Output "第 $($f.attempt) 次练习，接地 $($f.elapsed_s) s，垂直速度 $($f.vy_mps) m/s，$grade，进程工作集 $memory"
    foreach ($side in '前','后') {
        $candidate = if ($side -eq '前') {
            $frames | Where-Object { $_.Time -ge $start -and $_.Time -le $event.Time -and $_.Time -ge $event.Time-6 } | Select-Object -Last 1
        } else {
            $frames | Where-Object { $_.Time -gt $event.Time -and $_.Time -lt $end -and $_.Time -le $event.Time+6 } | Select-Object -First 1
        }
        if (!$candidate) { Write-Output "  接地${side}：六秒内无有效座舱采样"; continue }
        $r = $candidate.Row
        Write-Output "  接地${side}：$($r.elapsed_s) s，$($r.fps) FPS，P95 $($r.p95_ms) ms，$($r.samples) 帧，$($r.backend) $($r.render_width)×$($r.render_height)，FXAA=$($r.fxaa)，限帧=$($r.software_frame_cap)"
    }
}
Write-Output 'P95 来自截至该行时间的滚动窗口；工作集为接地时的进程内存。'
