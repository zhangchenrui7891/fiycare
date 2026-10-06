@echo off
setlocal
cd /d "%~dp0"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" goto missing
set "FCAIR_VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FCAIR_VS_PATH=%%i"
if not defined FCAIR_VS_PATH goto missing
call "%FCAIR_VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 goto failed
where cmake >nul 2>nul
if errorlevel 1 goto missing
cmake --preset windows-release
if errorlevel 1 goto failed
cmake --build --preset windows-release
if errorlevel 1 goto failed
ctest --preset windows-release
if errorlevel 1 goto failed
start "" out\build\release\FlightCareerAir.exe
exit /b 0
:missing
 echo Install Visual Studio Community and the Desktop development with C++ workload.
 echo Include the Windows SDK and C++ CMake tools.
 pause
 exit /b 1
:failed
 echo Build or tests failed. Copy the error text for diagnosis.
 pause
 exit /b 1
