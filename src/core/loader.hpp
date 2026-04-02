#pragma once

#include "database.hpp"
#include "ui/view.hpp"

enum class BCFileType {
    ELF, PE, BCTRACE, UNKNOWN
};

BCFileType detect_type(const std::string& path);

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv);