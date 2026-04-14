#pragma once

#include "address.hpp"
#include <cstdint>
#include <string>
#include <vector>

struct Neighbor {
    uint32_t id;
    std::string name;
    int count;
};

template <typename T>
struct RankedValue {
    T value;
    double percentile; // -1.0 means not applied
};

class BCDetailsViewModel;
class BCDatabase;

struct BCObject {
public:

    uint32_t id;
    std::string name;

    std::vector<Neighbor> prevs;
    std::vector<Neighbor> nexts;
    
    RankedValue<uint32_t> usage_count;

    BCObject(uint32_t id, const std::string& name)
        : id(id), name(name) {}
    virtual void dispatch_details(const BCDetailsViewModel& vm) const = 0;
    virtual ~BCObject() = default;
};


class BCBlock : public BCObject {
public:

    std::vector<BCObject*> members;

    RankedValue<uint32_t> instr_count;

    BCBlock(uint32_t id, std::vector<BCObject*> members)
        : BCObject(id, "BLK_" + std::to_string(id)), members(std::move(members)) {}

    void dispatch_details(const BCDetailsViewModel& vm) const;

};

class BCBasicBlock : public BCObject {
public:
    BCAddr address;
    BCBasicBlock(uint32_t id, BCAddr address)
        : BCObject(id, to_hex(address)), address(address) {}

    void dispatch_details(const BCDetailsViewModel& vm) const;

};
struct BCLoop : public BCObject {
public:
    std::vector<BCObject*> body;
    std::vector<uint32_t> raw_body;

    template<typename Iterator>
    BCLoop(uint32_t id, Iterator begin, Iterator end)
        : BCObject(id, "LOOP_" + std::to_string(id)), raw_body(begin, end) {}

    void dispatch_details(const BCDetailsViewModel& vm) const;
    
};

struct BCLoopInstance {
    uint32_t loop_id;
    uint32_t iterations;
};