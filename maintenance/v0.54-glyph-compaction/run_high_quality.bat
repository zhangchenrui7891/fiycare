@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist out\build\release\FlightCareerAir.exe (
 echo 请先运行 build_vs.bat 编译程序。
 pause
 exit /b 1
)
start "" out\build\release\FlightCareerAir.exe --fullscreen --supersample
