#pragma once

#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>
#include <map>

class BCBlock {
private:
    uint32_t id;
public:

    std::string name;
    std::vector<std::string> locs;

    uint16_t instr_count;
    uint32_t usage_count;

    std::map<uint32_t, uint32_t> prevs;
    std::map<uint32_t, uint32_t> nexts;

    virtual size_t loc_count() const {
        return locs.size();
    }

    virtual void info() const;

    uint32_t first_loc() const {
        return std::stoi(locs.front(), nullptr, 16);
    }

    uint32_t last_loc() const {
        return std::stoi(locs.back(), nullptr, 16);
    }

    bool check_loc(const std::string& target) const {
        return std::find(locs.begin(), locs.end(), target) != locs.end();
    }

    BCBlock() = default;

    BCBlock(uint32_t id, std::string name, std::vector<std::string> locs):
        id(id), name(name), locs(std::move(locs)) {}

    virtual ~BCBlock() = default;
};

class BCBasicBlock : public BCBlock {
public:

    size_t loc_count() const override {
        return 1;
    }

    void info() const override;

    BCBasicBlock(uint32_t id, std::string name) :
        BCBlock(id, std::move(name), { name }) {}

};