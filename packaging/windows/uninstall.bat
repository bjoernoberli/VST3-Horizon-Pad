@echo off
REM Horizon Pad - Windows uninstaller (self-elevating, same pattern as
REM install.bat). Removes the plugin from the shared VST3 location.

net session >nul 2>&1
if %errorLevel% == 0 (
    goto :main
) else (
    echo Requesting administrator rights...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

:main
set "DEST=%CommonProgramFiles%\VST3\Horizon Pad.vst3"

if not exist "%DEST%" (
    echo Horizon Pad is not installed at "%DEST%".
    pause
    exit /b 0
)

echo Removing "%DEST%" ...
rmdir /s /q "%DEST%"

echo.
echo Horizon Pad has been uninstalled.
echo.
pause
