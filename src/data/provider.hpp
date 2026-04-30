#pragma once

#include <capstone/capstone.h>
#include "core/address.hpp"
#include <cstdint>
#include <vector>

struct BCInstruction {
    uint64_t va;
    uint16_t size;
    std::string mnemonic;
    std::string op_str;
};
class BCCodeProvider {
public:
    
    virtual std::vector<uint8_t> get_instructions(BCAddr runtime_base, BCAddr va, size_t count) = 0;
    virtual std::vector<BCInstruction> get_bb(BCAddr runtime_base, BCAddr va) = 0;
    virtual ~BCCodeProvider() = default;

};