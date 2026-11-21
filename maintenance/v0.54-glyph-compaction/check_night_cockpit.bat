@echo off
chcp 65001 >nul
cd /d "%~dp0"
call "%~dp0tools\resolve_flight_exe.bat"
if not exist "%FCAIR_APP%" (
 echo 请先运行 build_vs.bat。
 pause
 exit /b 1
)
for %%B in (dx11 dx12) do (
 for %%F in (on off) do (
  if "%%F"=="off" (
   start /wait "" "%FCAIR_APP%" --%%B --smoke-test --a350 --cockpit --night --flood-off
  ) else (
   start /wait "" "%FCAIR_APP%" --%%B --smoke-test --a350 --cockpit --night
  )
  if errorlevel 1 goto failed
  findstr /c:"Smoke test: three rendered frames" "%LOCALAPPDATA%\FlightCareerAir\diagnostics\smoke\startup.log" >nul
  if errorlevel 1 goto failed
  if "%%B"=="dx12" (
   findstr /c:"DX12 adapter:" "%LOCALAPPDATA%\FlightCareerAir\diagnostics\smoke\startup.log" >nul
   if errorlevel 1 goto failed
  )
 )
 start /wait "" "%FCAIR_APP%" --%%B --smoke-test --a350 --cockpit --night --flood-off --panel-dim
 if errorlevel 1 goto failed
 findstr /c:"Smoke test: three rendered frames" "%LOCALAPPDATA%\FlightCareerAir\diagnostics\smoke\startup.log" >nul
 if errorlevel 1 goto failed
 if "%%B"=="dx12" (
  findstr /c:"DX12 adapter:" "%LOCALAPPDATA%\FlightCareerAir\diagnostics\smoke\startup.log" >nul
  if errorlevel 1 goto failed
 )
)
echo 六组夜景已渲染。截图：%%LOCALAPPDATA%%\FlightCareerAir\diagnostics\smoke\smoke_*_night_*.bmp
echo 请对比泛光开/关及仪表25%%/75%%亮度，人工检查中文、反光和外部地景。
echo 此脚本不构成画面验收或性能基准；测试不保存飞行状态。
pause
exit /b 0
:failed
echo 夜景检查失败或DX12回退到DX11。请查看 startup.log。
pause
exit /b 1
