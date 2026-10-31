# FlightCareerAir v0.45 Windows source snapshot

This contains core application source and root C++ checks. Binary runtime assets and Assimp sources are omitted; this is NOT the complete release mirror. The full delivered FlightCareerAir_v0.45.zip retains them.

On Windows/Visual Studio2022: pwsh -File tools/check_windows_source.ps1

The script explicitly disables Assimp and asset copying; it compiles the real main.cpp and DX11/DX12 classes and runs four real Windows CPU/HLSL checks. The application itself is NOT launched. Default CMake builds still enable Assimp and resource copying and therefore require the complete source package.

Actual successful run for product commit 25a66c6ee50f72ec4e44c6336d3124ee3feb282e: https://github.com/zhangchenrui7891/fiycare/actions/runs/41162739903

This final evidence commit changes only documentation/logs. Tested product source is unchanged. Windows default Assimp build, runtime graphics, input and HD6000 performance remain unverified. See WINDOWS_SOURCE_BUILD.md and PROGRESS.md.
