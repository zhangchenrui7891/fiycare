# FlightCareerAir v0.48 Assimp-enabled core source snapshot

On Windows/VS2022 with Python: run tools/check_windows_importer.ps1. It restores the exact bundled Assimp5.4.3 subset from a SHA256-pinned official archive, compiles the real application and importer with Assimp ON, and runs ten CPU/importer/HLSL/GDI checks. Existing local dependency modifications are refused; a complete matching vendor tree is never downloaded or replaced.

Successful product commit 23641d298f230b860e2fbf6573dd539dc9b19683: https://github.com/zhangchenrui7891/fiycare/actions/runs/41228015869

Existing importer Unicode path support was already present and is unchanged. This iteration adds reproducible dependency restoration, Assimp-enabled Windows compilation, and ASCII/Unicode external-texture import fixtures.

Runtime assets are omitted here and asset copying is OFF in this check. The full delivered FlightCareerAir_v0.48.zip retains all runtime assets and dependencies. No game startup, GPU/HD6000 performance or complete flight validation was performed. See ASSIMP_WINDOWS_BUILD.md and PROGRESS.md. This evidence commit changes documentation only.
