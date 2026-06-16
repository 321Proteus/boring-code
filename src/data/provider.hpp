#pragma once

#include "core/address.hpp"
#include "core/object.hpp"
#include <cstdint>
#include <memory>
#include <vector>

struct BCInstruction {
    uint64_t va;
    uint16_t size;
    std::string mnemonic;
    std::string op_str;
};
class BCCodeProvider {
public:
    
    BCAddr start, end;

    BCCodeProvider(BCAddr start, BCAddr end)
        : start(start), end(end) {}

    virtual std::vector<uint8_t> get_instructions(BCAddr runtime_base, BCAddr va, size_t count) = 0;
    virtual std::vector<BCInstruction> get_bb(BCAddr va) = 0;
    virtual ~BCCodeProvider() = default;

};

class BCCodeProviderRegistry {
public:

    BCCodeProvider* find(BCAddr addr) const;
    void register_provider(BCModule mod, std::unique_ptr<BCCodeProvider> prov);

private:
    struct Entry {
        BCAddr start;
        BCAddr end;
        // BCObjectId module_id;
        std::unique_ptr<BCCodeProvider> provider;
    };

    std::vector<Entry> mapping;
    mutable int last_index = -1;
};