@echo off
chcp 65001 >nul
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
cmake -DFCAIR_REPAIR=ON -P tools/repair_build_timestamps.cmake
if errorlevel 1 goto failed
cmake --preset windows-cockpit
if errorlevel 1 goto failed
cmake --build --preset windows-cockpit
if errorlevel 1 goto failed
if /i "%~1"=="--build-only" exit /b 0
start "" out\build\cockpit\FlightCareerAir.exe --dx11 --low --cockpit
exit /b 0
:missing
 echo 请安装 Visual Studio Community，并勾选“使用C++的桌面开发”。
 echo 需要包含 Windows SDK 和 C++ CMake 工具。
 if /i not "%~1"=="--build-only" pause
 exit /b 1
:failed
 echo 编译或检查失败，请保留以上错误信息。
 if /i not "%~1"=="--build-only" pause
 exit /b 1
