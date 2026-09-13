@echo off
setlocal enabledelayedexpansion

set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%\..\..
for %%I in ("%ROOT_DIR%") do set ROOT_DIR=%%~fI
set BUILD_DIR=%SCRIPT_DIR%\build

cmake -S "%ROOT_DIR%" -B "%BUILD_DIR%"
if errorlevel 1 exit /b 1
cmake --build "%BUILD_DIR%" --target network_tests --config Debug
if errorlevel 1 exit /b 1
"%BUILD_DIR%\Debug\network_tests.exe"

