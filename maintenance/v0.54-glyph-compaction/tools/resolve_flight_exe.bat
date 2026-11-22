@echo off
rem Compatibility launchers prefer the local cockpit build. Other callers prefer the full build.
set "FCAIR_APP=%~dp0..\out\build\cockpit\FlightCareerAir.exe"
if /i "%~1"=="cockpit" if exist "%FCAIR_APP%" exit /b 0
set "FCAIR_APP=%~dp0..\out\build\release\FlightCareerAir.exe"
if exist "%FCAIR_APP%" exit /b 0
set "FCAIR_APP=%~dp0..\out\build\cockpit\FlightCareerAir.exe"
if exist "%FCAIR_APP%" exit /b 0
set "FCAIR_APP=%~dp0..\FlightCareerAir.exe"
if exist "%FCAIR_APP%" exit /b 0
set "FCAIR_APP="
exit /b 1
