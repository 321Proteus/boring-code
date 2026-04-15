#pragma once

#include <cstdint>
#include <sstream>
#include <string>

using BCAddr = uint64_t;

inline std::string to_hex(BCAddr val) {
    std::stringstream ss;
    ss << std::hex << val;
    return ss.str();
}
