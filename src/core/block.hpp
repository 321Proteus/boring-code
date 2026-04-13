#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "object.hpp"
#include "address.hpp"

class BCDatabase;

class BCBlock : public BCObject {
public:

    std::vector<BCObject*> members;

    RankedValue<uint32_t> instr_count;

    BCBlock(uint32_t id, std::vector<BCObject*> members)
        : BCObject(id, "BLK_" + std::to_string(id)), members(std::move(members)) {}

};

class BCBasicBlock : public BCObject {
public:
    BCAddr address;
    BCBasicBlock(uint32_t id, BCAddr address)
        : BCObject(id, to_hex(address)), address(address) {}

};
struct BCLoop : public BCObject {
public:
    std::vector<BCObject*> body;
    std::vector<uint32_t> raw_body;

    template<typename Iterator>
    BCLoop(uint32_t id, Iterator begin, Iterator end)
        : BCObject(id, "LOOP_" + std::to_string(id)), raw_body(begin, end) {}
};

struct BCLoopInstance {
    uint32_t loop_id;
    uint32_t iterations;
};