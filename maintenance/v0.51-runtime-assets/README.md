# FlightCareerAir v0.49 native asset length preflight

Readers reject mismatched payload lengths before allocating large buffers. Valid FCTEX005/FCMESH06/07 formats and bounds remain compatible. See NATIVE_ASSET_LENGTHS.md and PROGRESS.md.

On Windows/VS2022 with Python: run tools/check_windows_importer.ps1. Exact pinned Assimp sources are restored when absent; main/importer compile with Assimp ON and resource copy OFF. Successful tested product commit 414f64bfae88ff17e1b93fcfa118f8133a5082fc: https://github.com/zhangchenrui7891/fiycare/actions/runs/41251384085

31 Linux tests and 11 Windows checks passed. Allocation-failure injection is test-only and is NOT process RAM, GPU or FPS evidence. Complete runtime assets are in the delivered FlightCareerAir_v0.49.zip, omitted from this source snapshot. No game startup, actual input or HD6000 runtime validation. This evidence commit changes documentation only.
