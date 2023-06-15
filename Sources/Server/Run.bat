@echo off
setlocal
set INSTALL_DIR=%~dp0..\..
set STREAMER_DIR=%INSTALL_DIR%\Engine\StreamingApp
Pushd %STREAMER_DIR%
set USD_ROOT=%INSTALL_DIR%\USD
set RPR=%USD_ROOT%
set PYTHONPATH=%USD_ROOT%\lib\python
set PXR_PLUGINPATH_NAME=%USD_ROOT%\plugin
set USD_PATHS=%USD_ROOT%\bin;%USD_ROOT%\lib;%USD_ROOT%\plugin\usd
set PATH=%USD_PATHS%;%PATH%
set PXR_USDMTLX_STDLIB_SEARCH_PATHS=%USD_ROOT%\libraries
set RENDERER_PATHS=%INSTALL_DIR%\Engine\UsdRenderer
set PATH=%RENDERER_PATHS%;%PATH%
%~dp0\RenderStudioServer.exe %*
