@echo off
title Fortnite SDK Dumper - Auto Compiler
echo ============================================================
echo   Fortnite SDK Dumper - Auto Build Script
echo ============================================================
echo.

:: Try to find Visual Studio installation
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo [!] Visual Studio not found.
    echo     Please install Visual Studio 2022 with "Desktop development with C++"
    echo     Download: https://visualstudio.microsoft.com/downloads/
    echo.
    pause
    exit /b 1
)

:: Get VS installation path
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo [!] Visual Studio C++ tools not found.
    echo     Open Visual Studio Installer and add "Desktop development with C++"
    echo.
    pause
    exit /b 1
)

echo [+] Found Visual Studio at: %VS_PATH%
echo.

:: Setup VS environment
call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if errorlevel 1 (
    echo [!] Failed to setup build environment
    pause
    exit /b 1
)

:: Check if cmake is available
where cmake >nul 2>&1
if errorlevel 1 (
    echo [!] CMake not found in PATH.
    echo     Checking VS installation for CMake...
    
    :: Try VS bundled CMake
    set "CMAKE_PATH=%VS_PATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    if exist "%CMAKE_PATH%\cmake.exe" (
        set "PATH=%CMAKE_PATH%;%PATH%"
        echo [+] Using VS bundled CMake
    ) else (
        echo [!] CMake not found. Install it from https://cmake.org/download/
        pause
        exit /b 1
    )
)

echo [+] CMake found
echo.

:: Create build directory
if not exist "build" mkdir build
cd build

:: Configure
echo [*] Configuring project...
echo.
cmake .. -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo.
    echo [!] CMake configuration failed.
    echo     Trying with Ninja generator instead...
    cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    
    if errorlevel 1 (
        echo.
        echo [!] Build configuration failed. Make sure you have VS 2022 with C++ tools.
        cd ..
        pause
        exit /b 1
    )
)

:: Build
echo.
echo [*] Building Release...
echo.
cmake --build . --config Release

if errorlevel 1 (
    echo.
    echo [!] Build FAILED. Check errors above.
    cd ..
    pause
    exit /b 1
)

:: Done
echo.
echo ============================================================
echo [+] BUILD SUCCESSFUL!
echo.

:: Find the exe
if exist "Release\FortniteDumper.exe" (
    echo     Output: %CD%\Release\FortniteDumper.exe
    echo.
    echo     Copying to project root...
    copy /Y "Release\FortniteDumper.exe" "..\FortniteDumper.exe" >nul
    echo     Copied to: FortniteDumper.exe
) else if exist "FortniteDumper.exe" (
    echo     Output: %CD%\FortniteDumper.exe
    echo.
    echo     Copying to project root...
    copy /Y "FortniteDumper.exe" "..\FortniteDumper.exe" >nul
    echo     Copied to: FortniteDumper.exe
) else (
    echo     [!] Could not find compiled exe. Check build output above.
)

echo ============================================================
echo.
cd ..
pause
