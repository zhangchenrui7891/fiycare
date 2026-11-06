param([string]$BuildDirectory = 'out/build/windows-source-check')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
Set-Location $project
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$build = (Resolve-Path $BuildDirectory).Path
Start-Transcript -Path (Join-Path $build 'windows-source-check.log') -Force | Out-Null
try {
    & cmake -S $project -B $build -G 'Visual Studio 17 2022' -A x64 -DFCAIR_WITH_ASSIMP=OFF -DFCAIR_COPY_RUNTIME_ASSETS=OFF
    if ($LASTEXITCODE -ne 0) { throw 'Source-only configuration failed' }
    & cmake --build $build --config Release --parallel 2 --target benchmark_check performance_segment_check model_restore_check autopilot_mode_check career_reset_check alert_focus_check map_clip_check instrument_format_check windows_glyph_check glyph_runs_check scenery_check FlightCareerAir shader_check windows_launch_check launch_options_check windows_text_check flight_instrument_check flight_instrument_layout_check autopilot_feedback_check autopilot_feedback_layout_check
    if ($LASTEXITCODE -ne 0) { throw 'Application or check compilation failed' }
    & ctest --test-dir $build -C Release --output-on-failure -R '^(benchmark_evidence_integrity|rolling_frame_configuration_boundaries|default_model_restore_transaction|cockpit_lateral_mode_transfer|career_flight_reset|alert_headline_priority|navigation_map_clipping|instrument_integer_format|windows_compact_glyph_coverage|compact_glyph_coverage|offline_scenery_geometry|hlsl_shader_compilation|windows_command_line_parsing|exact_launch_options|windows_utf8_device_log|primary_instrument_text|windows_instrument_glyph_bounds|autopilot_disconnect_feedback|windows_autopilot_notice_glyph_bounds)$'
    if ($LASTEXITCODE -ne 0) { throw 'Windows CPU checks failed' }
    if (!(Test-Path (Join-Path $build 'Release/FlightCareerAir.exe'))) { throw 'Application executable missing' }
    Write-Host 'Application compiled with Assimp OFF. Native fcmesh loading remains compiled. Runtime assets are NOT copied. No application/GPU/HD6000 execution was performed.'
} finally {
    Stop-Transcript | Out-Null
}
