#include <iomanip>
#include <string>
#include <sstream>
#include <variant>
#include "util.hpp"
#include "../address.hpp"

std::string to_hex(BCAddr val) {
    std::stringstream ss;
    std::visit([&ss](auto&& arg) {
        ss << std::setfill('0') << std::setw(sizeof(arg) * 2) << std::hex << arg;
    }, val);
    return ss.str();
}
