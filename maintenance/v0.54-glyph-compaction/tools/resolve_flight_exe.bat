@echo off
rem Prefer a local rebuild; otherwise use the packaged executable.
set "FCAIR_APP=%~dp0..\out\build\release\FlightCareerAir.exe"
if exist "%FCAIR_APP%" exit /b 0
set "FCAIR_APP=%~dp0..\FlightCareerAir.exe"
if exist "%FCAIR_APP%" exit /b 0
set "FCAIR_APP="
exit /b 1
