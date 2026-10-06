@echo off
cd /d "%~dp0"
if not exist out\build\release\FlightCareerAir.exe (
 echo Run build_vs.bat first.
 pause
 exit /b 1
)
echo Testing DX11: three frames and a BMP screenshot.
start /wait "" out\build\release\FlightCareerAir.exe --dx11 --smoke-test
if errorlevel 1 goto failed
echo Testing DX12: three frames and a BMP screenshot.
start /wait "" out\build\release\FlightCareerAir.exe --dx12 --smoke-test
if errorlevel 1 goto failed
echo Both graphics tests passed.
echo Inspect smoke_DX11.bmp and smoke_DX12.bmp in %%LOCALAPPDATA%%\FlightCareerAir.
pause
exit /b 0
:failed
 echo A graphics test failed. Read %%LOCALAPPDATA%%\FlightCareerAir\startup.log.
 echo If DX12 failed, use run_compat.bat for DX11 compatibility mode.
pause
exit /b 1
