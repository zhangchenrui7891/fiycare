@echo off
chcp 65001 >nul
setlocal
cd /d "%~dp0"
call "%~dp0tools\resolve_flight_exe.bat"
if not exist "%FCAIR_APP%" (
 echo Please run build_vs.bat first.
 pause
 exit /b 1
)
start "" "%FCAIR_APP%" --fullscreen --surface-flight
