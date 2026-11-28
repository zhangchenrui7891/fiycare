@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\report_practice.ps1" -Rollout %*
set "report_result=%errorlevel%"
pause
exit /b %report_result%
