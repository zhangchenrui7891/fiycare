@echo off
cd /d "%~dp0"
if not exist out\build\release\FlightCareerAir.exe (
 echo Run build_vs.bat first.
 pause
 exit /b 1
)
start "" out\build\release\FlightCareerAir.exe --dx11 --low
