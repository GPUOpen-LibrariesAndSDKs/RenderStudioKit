@echo off
setlocal

:: Get current script directory
set SCRIPT_DIR="%~dp0"

:: Prepare environment
set USD_DIR=%~dp0..
set SERVICES_DIR=%USD_DIR%\..
set PYTHON_DIR=%SERVICES_DIR%\Engine\Standalone

:: Setup environment
set USD_PATH=%USD_DIR%\lib;%USD_DIR%\bin;%USD_DIR%\plugin\usd
set PATH=%PATH%;%USD_PATH%;%PYTHON_DIR%

:: Launch server
"%~dp0\RenderStudioServer.exe" %*
