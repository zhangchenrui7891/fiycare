# FlightCareerAir v0.46 core source snapshot

Metric PFD units, fractional vertical speed and consistent 000-359 heading display. See PRIMARY_INSTRUMENTS.md and PROGRESS.md.

Run tools/check_windows_source.ps1 on Windows/VS2022. It compiles the actual application with Assimp and resource copying OFF, and runs six Windows CPU/HLSL/GDI checks. Successful tested product commit c6e789c4db51ff2b0096683c5fcd80b8c414f8a2: https://github.com/zhangchenrui7891/fiycare/actions/runs/41183414666

This final evidence commit changes documentation only. Binary runtime assets and third-party dependencies are omitted here; use the complete delivered FlightCareerAir_v0.46.zip for default builds. No application or GPU runtime, full 3D screen layout or HD6000 performance was tested.
