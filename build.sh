#!/bin/bash
set -e

# 32-bit
cmake -B build -S tools \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-m32" \
    -DCMAKE_CXX_FLAGS="-m32"

cmake --build build --parallel $(nproc)

# 64-bit
cmake -B build64 -S . \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build64 --parallel $(nproc)