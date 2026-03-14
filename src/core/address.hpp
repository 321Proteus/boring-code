#pragma once

#include <cstdint>

#define _PLATFORM_X86 true

#ifdef _PLATFORM_X86
typedef uint32_t BCAddr;
constexpr std::size_t ADDR_SIZE = 4;
#else
typedef uin64_t BCAddr;
constexpr std::size_t ADDR_SIZE = 8;
#endif