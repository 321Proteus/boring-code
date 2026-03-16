#pragma once

#include "core/provider.hpp"
#include <vector>

class DirectCodeProvider : public BCCodeProvider {
public:

    std::vector<uint8_t> get_instruction(uint64_t address) override {
        return { 'a', 'b', 'c' };
    }

};