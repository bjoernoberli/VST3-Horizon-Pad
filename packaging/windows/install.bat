@echo off
REM Horizon Pad - Windows installer
REM
REM Installs to the shared VST3 location (C:\Program Files\Common
REM Files\VST3\), which requires admin rights. Rather than asking the user to
REM right-click and "Run as administrator", this script self-elevates: it
REM checks whether it is already running elevated, and if not, relaunches
REM itself via PowerShell's Start-Process -Verb RunAs (a single UAC prompt),
REM then exits the original non-elevated instance.
REM
REM Expects "Horizon Pad.vst3" to sit next to this script (that's how it
REM ships in the release zip).

net session >nul 2>&1
if %errorLevel% == 0 (
    goto :main
) else (
    echo Requesting administrator rights...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

:main
setlocal

set "SRC=%~dp0Horizon Pad.vst3"
set "DEST=%CommonProgramFiles%\VST3\Horizon Pad.vst3"

if not exist "%SRC%" (
    echo ERROR: "Horizon Pad.vst3" was not found next to this installer.
    echo Make sure install.bat stays in the same folder as "Horizon Pad.vst3".
    pause
    exit /b 1
)

echo Installing Horizon Pad to "%DEST%" ...

if exist "%DEST%" (
    rmdir /s /q "%DEST%"
)

mkdir "%CommonProgramFiles%\VST3" >nul 2>&1
xcopy /e /i /y "%SRC%" "%DEST%" >nul

if %errorLevel% neq 0 (
    echo ERROR: Installation failed. See the messages above.
    pause
    exit /b 1
)

echo.
echo Horizon Pad installed successfully.
echo Rescan plugins in your DAW to pick it up.
echo.
pause
