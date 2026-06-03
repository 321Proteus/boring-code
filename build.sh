#!/bin/bash
set -e

if [[ -z "$1" ]]; then
    echo "Usage: ./build.sh <preset> [--rebuild]"
    echo "Presets: linux-x64, linux-x86"
    exit 1
fi

PRESET=$1

if [[ "$2" == "--rebuild" ]]; then
    echo "Removing old build directory"
    rm -rf build/$PRESET
fi

cmake --preset $PRESET -DDynamoRIO_DIR=/opt/drio/cmake
cmake --build build/$PRESET --parallel $(nproc)

echo "Updating .clangd..."
cp .clangd_template .clangd
echo "  CompilationDatabase: build/$PRESET" >> .clangd