#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>
#include <map>

#include "object.hpp"
#include "address.hpp"

class BCDatabase;

typedef struct {
    uint32_t id;
    std::string name;
    size_t count;
} Neighbor;

template <typename T>
struct RankedValue {
    T value;
    double percentile; // -1.0 means not applied
};

class BCBlock : public BCObject {
public:

    std::vector<uint32_t> members;

    RankedValue<uint32_t> instr_count;
    RankedValue<uint32_t> usage_count;

    std::map<uint32_t, uint32_t> prevs;
    std::map<uint32_t, uint32_t> nexts;

    virtual size_t member_count() const {
        return members.size();
    }

    uint32_t first_member() const {
        return members.front();
    }

    uint32_t last_member() const {
        return members.back();
    }

    bool check_member(uint32_t target) const {
        return std::find(members.begin(), members.end(), target) != members.end();
    }

    BCBlock(uint32_t id, std::vector<uint32_t> members)
        : BCObject(id, "BLK_" + std::to_string(id)), members(std::move(members)) {}

    typedef struct {
        uint32_t id;
        std::string name;
        RankedValue<uint32_t> usage_count;
        std::vector<uint32_t> members;
        std::vector<Neighbor> prevs;
        std::vector<Neighbor> nexts;
    } Details;

    virtual ~BCBlock() = default;
};

class BCBasicBlock : public BCObject {
public:
    BCAddr address;
    BCBasicBlock(uint32_t id, BCAddr address)
        : BCObject(id, std::to_string(address)), address(address) {}

};
struct BCLoop : public BCObject {
public:
    std::vector<uint32_t> body;

    template<typename Iterator>
    BCLoop(uint32_t id, Iterator begin, Iterator end)
        : BCObject(id, "LOOP_" + std::to_string(id)), body(begin, end) {}
};

struct BCLoopInstance {
    uint32_t loop_id;
    uint32_t iterations;
};