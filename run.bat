@echo off
setlocal

set "APP_DIR=%~dp0build\Release"
set "APP_EXE=%APP_DIR%\phase-correlation-tester.exe"

if not exist "%APP_EXE%" (
    echo Phase Correlation Tester has not been built yet.
    echo Configure and build the project first; see README.md for the Windows commands.
    pause
    exit /b 1
)

pushd "%APP_DIR%"
"%APP_EXE%"
set "APP_EXIT=%ERRORLEVEL%"
popd

if not "%APP_EXIT%"=="0" pause
exit /b %APP_EXIT%
