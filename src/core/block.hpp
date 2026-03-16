#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>
#include <map>

#include "address.hpp"

class BCDatabase;

class BCBlock {
private:
    uint32_t id;
public:

    std::string name;
    std::vector<BCAddr> locs;

    uint16_t instr_count;
    uint32_t usage_count;

    std::map<uint32_t, uint32_t> prevs;
    std::map<uint32_t, uint32_t> nexts;

    virtual size_t loc_count() const {
        return locs.size();
    }

    virtual std::string info(const BCDatabase& db) const;

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

    virtual ~BCBlock() = default;
};

class BCBasicBlock : public BCBlock {
public:

    size_t loc_count() const override {
        return 1;
    }

    std::string info(const BCDatabase& db) const override;

    BCBasicBlock(uint32_t id, std::string address) :
        BCBlock(id, address, {}) {
            locs.push_back(static_cast<BCAddr>(std::stoi(address, 0, 16)));
        }

};