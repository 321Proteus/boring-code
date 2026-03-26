#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>

#include "address.hpp"
#include "block.hpp"

class BCBlock;

class BCDatabase {
public:
    std::unordered_map<std::string, uint32_t> names;
    std::unordered_map<uint32_t, std::unique_ptr<BCBlock>> blocks;
    std::vector<uint32_t> trace;
    
    std::unordered_map<BCAddr, std::map<BCAddr, int>> next_map;
    std::unordered_map<BCAddr, std::map<BCAddr, int>> prev_map;

    template<typename T, typename... Args>
    void insert(uint32_t id, const std::string& name, Args&&... args) {

        if (blocks.find(id) != blocks.end()) {
            throw std::runtime_error("Block already exists");
            return;
        }

        if (names.find(name) != names.end()) {
            throw std::runtime_error("Name already exists");
            return;
        }

        auto block = std::make_unique<T>(id, name, std::forward<Args>(args)...);
        block->name = name;

        names[name] = id;
        blocks.emplace(id, std::move(block));
    }

    bool rename(uint32_t id, std::string newName);

    BCBlock* getByName(const std::string& name) const {
        auto it = names.find(name);
        if (it == names.end()) return nullptr;
        return blocks.at(it->second).get();
    }

    BCBlock* getById(uint32_t id) const {
        auto it = blocks.find(id);
        if (it == blocks.end()) return nullptr;
        return it->second.get();
    }

    BCBlock::Details generate_details(const BCBlock& block) const;

    BCBlock* getByLoc(BCAddr address) const;

    void apply_prevs_nexts();
    void apply_trace(const std::vector<uint32_t>& trace);

    const float HOT_COLD_THRESHOLD = 0.2;

    void find_hot_cold_blocks();

    BCDatabase() = default;

};