# FlightCareerAir v0.47 core source snapshot

New existing-notice feedback for actual autopilot channel losses from pilot override and protection/power transitions. Autopilot control law and save format remain byte-identical to v0.46. See AUTOPILOT_FEEDBACK.md and PROGRESS.md.

Run tools/check_windows_source.ps1 on Windows/VS2022. It compiles the actual application with Assimp/resource copying OFF and runs eight CPU/HLSL/GDI checks. Successful product commit 1f5459f05283428a864fe1203ea6910d0aeb2615: https://github.com/zhangchenrui7891/fiycare/actions/runs/41204391637

This evidence commit changes documentation only. Runtime assets and third-party sources are omitted here; complete defaults require the delivered FlightCareerAir_v0.47.zip. No game, actual keyboard/mouse, GPU or HD6000 runtime was tested.
