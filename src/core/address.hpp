#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <variant>

using BCAddr = std::variant<uint32_t, uint64_t>;

inline std::string to_hex(BCAddr val) {
    std::stringstream ss;
    std::visit([&ss](auto&& arg) {
        ss << std::hex << arg;
    }, val);
    return ss.str();
}
