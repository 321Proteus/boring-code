@echo off
setlocal enabledelayedexpansion

REM 32-bit build
cmake -B build -S tools -A Win32 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DDynamoRIO_DIR="C:/drio/cmake" ^
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmake --build build --parallel %NUMBER_OF_PROCESSORS%

REM 64-bit build
cmake -B build64 -S . -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DDynamoRIO_DIR="C:/drio/cmake" ^
    -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
    -DQt6_DIR="C:\Qt\6.11.0\msvc2022_64\lib\cmake\Qt6"

cmake --build build64 --config Release --parallel %NUMBER_OF_PROCESSORS%

endlocal
