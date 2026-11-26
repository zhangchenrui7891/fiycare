@echo off
chcp 65001 >nul
cd /d "%~dp0"
call "%~dp0tools\resolve_flight_exe.bat" cockpit
if not exist "%FCAIR_APP%" (
 echo 请先运行 build_vs.bat 编译程序。
 pause
 exit /b 1
)
start "" "%FCAIR_APP%" --dx11 --low --practice-a330-rollout
