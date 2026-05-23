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

:: ============================================================
:: Download ImGui if not present
:: ============================================================
if not exist "libs\imgui\imgui.cpp" (
    echo [*] Downloading ImGui...
    
    if not exist "libs" mkdir libs
    
    :: Use PowerShell to download ImGui zip
    powershell -Command "& { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri 'https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.zip' -OutFile 'libs\imgui.zip' }" 2>nul
    
    if not exist "libs\imgui.zip" (
        echo [!] Failed to download ImGui. Trying alternative...
        powershell -Command "& { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; (New-Object System.Net.WebClient).DownloadFile('https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.zip', 'libs\imgui.zip') }" 2>nul
    )
    
    if not exist "libs\imgui.zip" (
        echo [!] Failed to download ImGui.
        echo     Please download manually from: https://github.com/ocornut/imgui/releases/tag/v1.91.8
        echo     Extract to: libs\imgui\
        pause
        exit /b 1
    )
    
    :: Extract zip
    echo [*] Extracting ImGui...
    powershell -Command "& { Expand-Archive -Path 'libs\imgui.zip' -DestinationPath 'libs\' -Force }"
    
    :: Rename extracted folder
    if exist "libs\imgui-1.91.8" (
        if exist "libs\imgui" rmdir /s /q "libs\imgui"
        rename "libs\imgui-1.91.8" "imgui"
    )
    
    :: Cleanup zip
    del /q "libs\imgui.zip" 2>nul
    
    if exist "libs\imgui\imgui.cpp" (
        echo [+] ImGui downloaded and extracted successfully!
    ) else (
        echo [!] ImGui extraction failed.
        pause
        exit /b 1
    )
    echo.
)

:: ============================================================
:: Build
:: ============================================================

:: Clean old build if cmake config changed
if exist "build\CMakeCache.txt" (
    findstr /c:"FetchContent" "build\CMakeCache.txt" >nul 2>&1
    if not errorlevel 1 (
        echo [*] Cleaning old build directory...
        rmdir /s /q build
    )
)

if not exist "build" mkdir build
cd build

:: Configure
echo [*] Configuring project...
echo.
cmake .. -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo.
    echo [!] CMake configuration failed.
    cd ..
    pause
    exit /b 1
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
