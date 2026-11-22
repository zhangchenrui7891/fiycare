@echo off
setlocal
call "%~dp0tools\resolve_flight_exe.bat" cockpit
set "EXE=%FCAIR_APP%"
set "REPORTS=%LOCALAPPDATA%\FlightCareerAir\diagnostics\benchmark"
if not exist "%EXE%" (
  echo EXE not found. Run build_vs.bat first.
  pause
  exit /b 1
)
echo Keep each simulator window in front for about 60 seconds. Do not change settings.
echo Running 777 cockpit on DX11 with FXAA on, then off.
start "" /wait "%EXE%" --dx11 --benchmark --benchmark-777-cockpit
if not "%errorlevel%"=="0" goto failed
findstr /x /c:"result_status=VALID" "%REPORTS%\777-cockpit\performance_summary.txt" >nul
if errorlevel 1 goto failed
start "" /wait "%EXE%" --dx11 --benchmark --benchmark-777-cockpit --no-aa
if not "%errorlevel%"=="0" goto failed
findstr /x /c:"result_status=VALID" "%REPORTS%\no-fxaa\777-cockpit\performance_summary.txt" >nul
if errorlevel 1 goto failed
echo FXAA on:  "%REPORTS%\777-cockpit"
echo FXAA off: "%REPORTS%\no-fxaa\777-cockpit"
pause
exit /b 0
:failed
echo A run did not produce a valid benchmark. Check startup.log and performance_summary.txt.
echo Reports root: "%REPORTS%"
pause
exit /b 1
