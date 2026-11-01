param([string]$BuildDirectory = 'out/build/windows-importer-check')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
Set-Location $project
New-Item -ItemType Directory -Force -Path $BuildDirectory | Out-Null
$build = (Resolve-Path $BuildDirectory).Path
Start-Transcript -Path (Join-Path $build 'windows-importer-check.log') -Force | Out-Null
try {
    & python tools/restore_assimp_ci.py
    if ($LASTEXITCODE -ne 0) { throw 'Pinned Assimp restoration failed' }
    & cmake -S $project -B $build -G 'Visual Studio 17 2022' -A x64 -DFCAIR_WITH_ASSIMP=ON -DFCAIR_COPY_RUNTIME_ASSETS=OFF
    if ($LASTEXITCODE -ne 0) { throw 'Assimp-enabled configuration failed' }
    & cmake --build $build --config Release --parallel 2 --target scenery_check FlightCareerAir asset_import asset_fixture_check shader_check windows_launch_check launch_options_check windows_text_check flight_instrument_check flight_instrument_layout_check autopilot_feedback_check autopilot_feedback_layout_check asset_length_check
    if ($LASTEXITCODE -ne 0) { throw 'Assimp-enabled application or checks failed to compile' }
    & ctest --test-dir $build -C Release --output-on-failure -R '^(offline_scenery_geometry|native_asset_payload_lengths|self_contained_asset_import|pinned_assimp_source_restore|hlsl_shader_compilation|windows_command_line_parsing|exact_launch_options|windows_utf8_device_log|primary_instrument_text|windows_instrument_glyph_bounds|autopilot_disconnect_feedback|windows_autopilot_notice_glyph_bounds)$'
    if ($LASTEXITCODE -ne 0) { throw 'Windows importer/CPU checks failed' }
    if (!(Test-Path (Join-Path $build 'Release/FlightCareerAir.exe'))) { throw 'Application executable missing' }
    Write-Host 'Application compiled with Assimp ON and exact pinned bundled sources. CPU import fixtures and checks passed. Runtime assets NOT copied; application/GPU/HD6000 NOT run.'
} finally {
    Stop-Transcript | Out-Null
}
