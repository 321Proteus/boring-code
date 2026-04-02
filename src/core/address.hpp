#pragma once

#include <cstdint>
#include <variant>

using BCAddr = std::variant<uint32_t, uint64_t>;