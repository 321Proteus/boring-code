#pragma once

#include "database.hpp"
#include "ui/view.hpp"

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv);