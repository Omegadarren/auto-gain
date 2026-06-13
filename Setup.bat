@echo off
setlocal
net session >nul 2>&1
if %errorlevel% equ 0 goto :run
echo Requesting administrator privileges...
powershell -Command "Start-Process powershell -ArgumentList '-NoProfile -ExecutionPolicy Bypass -File ""%~dp0setup.ps1""' -Verb RunAs"
exit /b
:run
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1"
if %errorlevel% neq 0 ( echo. & echo Setup failed. See the messages above. & pause )
