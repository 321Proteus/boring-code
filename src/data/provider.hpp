#pragma once

#include <cstdint>
#include <vector>

class BCCodeProvider {
public:
    
    virtual std::vector<uint8_t> get_instructions(uint64_t address, size_t count) = 0;
    virtual bool is_valid(uint64_t address) = 0;
    virtual ~BCCodeProvider() = default;

};