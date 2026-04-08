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

    std::vector<BCAddr> locs;

    RankedValue<uint32_t> instr_count;
    RankedValue<uint32_t> usage_count;

    std::map<uint32_t, uint32_t> prevs;
    std::map<uint32_t, uint32_t> nexts;

    virtual size_t loc_count() const {
        return locs.size();
    }

    BCAddr first_loc() const {
        return locs.front();
    }

    BCAddr last_loc() const {
        return locs.back();
    }

    bool check_loc(BCAddr target) const {
        return std::find(locs.begin(), locs.end(), target) != locs.end();
    }

    BCBlock(uint32_t id, const std::string& name, std::vector<BCAddr> locs)
        : BCObject(id, name), locs(std::move(locs)) {}

    typedef struct {
        uint32_t id;
        std::string name;
        RankedValue<uint32_t> usage_count;
        std::vector<BCAddr> locs;
        std::vector<Neighbor> prevs;
        std::vector<Neighbor> nexts;
    } Details;

    virtual ~BCBlock() = default;
};

class BCBasicBlock : public BCBlock {
public:

    BCBasicBlock(uint32_t id, const std::string& address)
        : BCBlock(id, address, { std::stoull(address, nullptr, 16) }) {}

};
struct BCLoop : public BCObject {
public:
    std::vector<uint32_t> body;

    template<typename Iterator>
    BCLoop(uint32_t id, const std::string& name, Iterator begin, Iterator end)
        : BCObject(id, name), body(begin, end) {}
};

struct BCLoopInstance {
    uint32_t loop_id;
    uint32_t iterations;
};