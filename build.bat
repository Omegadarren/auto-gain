@echo off
setlocal
set "DIR=%~dp0"
cmake --build "%DIR%build" --config Release --parallel
if %errorlevel% neq 0 ( echo Build failed! & pause )
