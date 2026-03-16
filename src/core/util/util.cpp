#include <string>
#include <sstream>
#include "util.hpp"
#include "../address.hpp"

std::string to_hex(BCAddr val) {
    std::stringstream ss;
    ss << std::hex << val;
    return ss.str();
}
