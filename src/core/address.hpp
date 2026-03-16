#pragma once

#include <cstdint>

// #define _PLATFORM_X64

#ifndef _PLATFORM_X64
typedef uint32_t BCAddr;
constexpr std::size_t ADDR_SIZE = 4;
#else
typedef uint64_t BCAddr;
constexpr std::size_t ADDR_SIZE = 8;
#endif