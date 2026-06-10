@echo off
setlocal enabledelayedexpansion

set QT6_PATH="C:\Qt\6.11.1\msvc2022_64"
set VCPKG_PATH="C:\vcpkg"
set DYNAMORIO_PATH="C:\drio"
set VULKAN_PATH="C:\VulkanSDK\1.4.350.0"

if "%1"=="" (
    echo Usage: build.bat ^<preset^> [--rebuild]
    echo Presets: windows-x64, windows-x86
    exit /b 1
)

set PRESET=%1

set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist %VSWHERE% (
    echo vswhere.exe not found - are you sure Visual Studio is installed?
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -property installationPath`) do (
    set VS_PATH=%%i
)

if "%PRESET%"=="windows-x64" (
    set VCVARS="%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
) else if "%PRESET%"=="windows-x86" (
    set VCVARS="%VS_PATH%\VC\Auxiliary\Build\vcvars32.bat"
) else (
    echo Unknown preset: %PRESET%
    exit /b 1
)

if not exist %VCVARS% (
    echo vcvars not found at %VCVARS%
    exit /b 1
)

echo Setting up VS environment from %VCVARS%...
call %VCVARS%
if errorlevel 1 exit /b 1

if "%2"=="--rebuild" (
    echo Removing old build directory
    rmdir /s /q build\%PRESET%
)

cmake --preset %PRESET% ^
    -DDynamoRIO_DIR="%DYNAMORIO_PATH%/cmake" ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake" ^
    -DQT_DIR="%QT6_PATH%\lib\cmake\Qt6" ^
    -DQt6_DIR="%QT6_PATH%\lib\cmake\Qt6" ^
    -DVulkan_INCLUDE_DIR="%VULKAN_PATH%\cmake"

if errorlevel 1 exit /b 1

cmake --build build\%PRESET% --parallel %NUMBER_OF_PROCESSORS%
if errorlevel 1 exit /b 1

echo Updating .clangd...
copy .clangd_template .clangd
echo   CompilationDatabase: build/%PRESET% >> .clangd