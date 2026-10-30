# FlightCareerAir v0.44 change set

Standalone launch module regression and integration sources, not a full simulator mirror. Full resources/dependencies are in FlightCareerAir_v0.44.zip. Apply validation_v044/changes_from_v043.patch only to the complete v0.43 baseline; also add the new headers/tests/tool. Do not apply to old v0.3.

Build this directory with CMake to run the two small Windows tests. They passed on commit d2bf7a1f09656d17a94cb938168e4bd6659fd07b: https://github.com/zhangchenrui7891/fiycare/actions/runs/41135044245

The tested headers and tests are unchanged in this documentation/integration commit. Full main.cpp, DirectX and HD6000 are not verified by this workflow. See LAUNCH_OPTIONS.md and PROGRESS.md.
