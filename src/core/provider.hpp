#pragma once

#include <cstdint>
#include <vector>

class BCCodeProvider {
public:
    
    virtual ~BCCodeProvider() = default;
    virtual std::vector<uint8_t> get_instruction(uint64_t address) = 0;

};