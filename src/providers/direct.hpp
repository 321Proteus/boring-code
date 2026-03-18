#pragma once

#include "data/provider.hpp"
#include <vector>

class DirectCodeProvider : public BCCodeProvider {
private:
    std::vector<uint8_t> binary;
    uint64_t base;
public:

    DirectCodeProvider(const std::vector<uint8_t>& binary, uint64_t base)
        : binary(std::move(binary)), base(base) {}

    std::vector<uint8_t> get_instructions(uint64_t address, size_t count) override {
        size_t offset = address - base;
        return std::vector<uint8_t>(
            binary.begin() + offset,
            binary.begin() + offset + count
        );
    }

};