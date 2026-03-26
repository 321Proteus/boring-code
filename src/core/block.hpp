#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>
#include <map>

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

class BCBlock {
private:
    uint32_t id;
public:

    std::string name;
    std::vector<BCAddr> locs;

    RankedValue<uint32_t> instr_count;
    RankedValue<uint32_t> usage_count;

    std::map<uint32_t, uint32_t> prevs;
    std::map<uint32_t, uint32_t> nexts;

    virtual size_t loc_count() const {
        return locs.size();
    }

    uint32_t get_id() const {
        return id;
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

    BCBlock() = default;

    BCBlock(uint32_t id, std::string name, std::vector<BCAddr> locs):
        id(id), name(name), locs(std::move(locs)) {}

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

    size_t loc_count() const override {
        return 1;
    }

    BCBasicBlock(uint32_t id, std::string address) :
        BCBlock(id, address, {}) {
            locs.push_back(static_cast<BCAddr>(std::stoi(address, 0, 16)));
        }

};