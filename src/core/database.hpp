#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <map>

#include "address.hpp"
#include "block.hpp"
#include "loader.hpp"
#include "ui/view.hpp"
#include "util/hash.h"

class BCBlock;

class BCDatabase {
public:
    std::unordered_map<std::string, uint32_t> names;
    std::unordered_map<uint32_t, std::unique_ptr<BCBlock>> blocks;
    std::unordered_map<uint32_t, std::unique_ptr<BCLoop>> loops;
    BCTrace trace;

    std::unordered_map<uint64_t, uint32_t> loop_hashes;
    
    std::unordered_map<BCAddr, std::map<BCAddr, int>> next_map;
    std::unordered_map<BCAddr, std::map<BCAddr, int>> prev_map;

    uint64_t base_address;
    uint32_t crc_hash;

    template<typename T, typename... Args>
    void insert(uint32_t id, const std::string& name, Args&&... args) {

        if (names.find(name) != names.end()) {
            throw std::runtime_error("Name already exists: " + name);
            return;
        }

        std::unique_ptr<T> obj = std::make_unique<T>(id, name, std::forward<Args>(args)...);
        names[name] = id;

        if constexpr (std::is_base_of_v<BCBlock, T>) {

            if (blocks.find(id) != blocks.end()) {
                throw std::runtime_error("Block already exists");
            }
            blocks.emplace(id, std::move(obj));

        } else if constexpr (std::is_same_v<T, BCLoop>) {

            BCLoop* loop = obj.get();

            uint64_t h = hash_window(loop->body.data(), loop->body.size());
            auto it = loop_hashes.find(h);
            if (it != loop_hashes.end()) {
                if (loops.at(it->second)->body == loop->body) {
                    throw std::runtime_error("Loop already exists: " + std::to_string(it->second));
                }
            }

            loop_hashes.emplace(h, id);
            loops.emplace(id, std::move(obj));

        } else {
            throw std::runtime_error("Incorrect type");
        }

    }

    bool rename(uint32_t id, std::string newName);

    BCBlock* getBlockByName(const std::string& name) const {
        auto it = names.find(name);
        return (it != names.end()) ? getBlockById(it->second) : nullptr;
    }

    BCLoop* getLoopByName(const std::string& name) const {
        auto it = names.find(name);
        return (it != names.end()) ? getLoopById(it->second) : nullptr;
    }

    BCBlock* getBlockById(uint32_t id) const {
        auto it = blocks.find(id);
        return (it != blocks.end()) ? it->second.get() : nullptr;
    }

    BCLoop* getLoopById(uint32_t id) const {
        auto it = loops.find(id);
        return (it != loops.end()) ? it->second.get() : nullptr;
    }

    BCBlock::Details generate_details(const BCBlock& block) const;

    BCBlock* getByLoc(BCAddr address) const;

    void apply_prevs_nexts();
    void apply_trace(const BCTrace& trace);

    const float HOT_COLD_THRESHOLD = 0.2;

    void find_hot_cold_blocks(BCStatusViewModel& sv);

    BCDatabase() = default;

};