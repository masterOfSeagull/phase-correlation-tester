@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT_DIR=%~dp0"
set "BUILD_DIR=%ROOT_DIR%build"
set "APP_DIR=%~dp0build\Release"
set "APP_EXE=%APP_DIR%\phase-correlation-tester.exe"
set "APP_LOG=%APP_DIR%\phase-correlation-tester.log"
set "CMAKE_EXE=%CMAKE_EXE%"

if not defined CMAKE_EXE (
    for %%I in (cmake.exe) do set "CMAKE_EXE=%%~$PATH:I"
)
if not defined CMAKE_EXE if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_EXE=C:\Program Files\CMake\bin\cmake.exe"
if not defined CMAKE_EXE if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not defined CMAKE_EXE if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_EXE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if not defined CMAKE_EXE (
    echo CMake was not found. Install CMake or set CMAKE_EXE to cmake.exe.
    pause
    exit /b 1
)

if not exist "%BUILD_DIR%\CMakeCache.txt" (
    set "QT_PREFIX=%CMAKE_PREFIX_PATH%"
    if not defined QT_PREFIX set "QT_PREFIX=C:/Qt/6.9.2/msvc2022_64"

    set "VCPKG_TOOLCHAIN=%CMAKE_TOOLCHAIN_FILE%"
    if not defined VCPKG_TOOLCHAIN if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" set "VCPKG_TOOLCHAIN=C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

    echo Configuring Phase Correlation Tester...
    if defined VCPKG_TOOLCHAIN (
        "%CMAKE_EXE%" -S "%ROOT_DIR%." -B "%BUILD_DIR%" -DCMAKE_PREFIX_PATH="!QT_PREFIX!" -DCMAKE_TOOLCHAIN_FILE="!VCPKG_TOOLCHAIN!"
    ) else (
        "%CMAKE_EXE%" -S "%ROOT_DIR%." -B "%BUILD_DIR%" -DCMAKE_PREFIX_PATH="!QT_PREFIX!"
    )
    if errorlevel 1 (
        echo.
        echo Configure failed.
        pause
        exit /b 1
    )
)

echo Building Phase Correlation Tester...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --config Release
if errorlevel 1 (
    echo.
    echo Build failed.
    pause
    exit /b 1
)

if not exist "%APP_EXE%" (
    echo.
    echo Build finished, but "%APP_EXE%" was not found.
    pause
    exit /b 1
)

pushd "%APP_DIR%"
"%APP_EXE%"
set "APP_EXIT=%ERRORLEVEL%"
popd

if not "%APP_EXIT%"=="0" (
    if exist "%APP_LOG%" (
        echo.
        echo Startup log:
        type "%APP_LOG%"
    )
    pause
)
exit /b %APP_EXIT%
