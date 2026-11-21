@echo off
chcp 65001 >nul
cd /d "%~dp0"
call "%~dp0tools\resolve_flight_exe.bat"
if not exist "%FCAIR_APP%" (
 echo 请先运行 build_vs.bat 编译程序。
 pause
 exit /b 1
)
for %%B in (dx11 dx12) do (
 for %%P in (a330 a350) do (
  echo 正在检查 %%B 和 %%P ：三帧卫星底图与客机截图。
  start /wait "" "%FCAIR_APP%" --%%B --smoke-test --%%P
  if errorlevel 1 goto failed
 )
)
echo 正在检查傍晚光照。
start /wait "" "%FCAIR_APP%" --dx11 --smoke-test --a350 --sunset
if errorlevel 1 goto failed
start /wait "" "%FCAIR_APP%" --dx12 --smoke-test --a350 --sunset
if errorlevel 1 goto failed
echo 正在检查三维驾驶舱与FXAA。
start /wait "" "%FCAIR_APP%" --dx11 --smoke-test --a350 --cockpit
if errorlevel 1 goto failed
start /wait "" "%FCAIR_APP%" --dx12 --smoke-test --a350 --cockpit
if errorlevel 1 goto failed
for %%B in (dx11 dx12) do (
 start /wait "" "%FCAIR_APP%" --%%B --smoke-test --a350 --cockpit --no-aa
 if errorlevel 1 goto failed
)
echo 十组检查程序均已退出；请查看截图确认中文、卫星底图及飞机外形。
echo 截图： %%LOCALAPPDATA%%\FlightCareerAir\diagnostics\smoke\smoke_DX11_2_noon_flood_bright_fxaa.bmp 等。
pause
exit /b 0
:failed
 echo 图形检查失败。查看 %%LOCALAPPDATA%%\FlightCareerAir\diagnostics\smoke\startup.log。
 echo DX12失败时可用 run_compat.bat 启动DX11兼容模式。
pause
exit /b 1
