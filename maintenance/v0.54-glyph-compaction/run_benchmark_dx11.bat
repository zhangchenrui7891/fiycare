@echo off
setlocal
call "%~dp0tools\resolve_flight_exe.bat" cockpit
set "EXE=%FCAIR_APP%"
if not exist "%EXE%" (
  echo EXE not found. Run build_vs.bat first.
  pause
  exit /b 1
)
echo Keep the benchmark window in front for 60 seconds.
echo Sampling errors may end early. Read result_status; normal exit is not a PASS.
"%EXE%" --dx11 --low --benchmark
echo Results: %%LOCALAPPDATA%%\FlightCareerAir\diagnostics\benchmark\performance_summary.txt
echo Screenshot: %%LOCALAPPDATA%%\FlightCareerAir\diagnostics\benchmark\benchmark_final.bmp
echo Startup log is in the same benchmark directory. Live game saves are not modified.
pause
