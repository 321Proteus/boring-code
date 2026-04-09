#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <vector>

#include "address.hpp"
#include "block.hpp"
#include "loader.hpp"
#include "ui/view.hpp"
#include "util/hash.h"

class BCBlock;

struct BCInsertionResult {
    uint32_t id;
    bool created;
};

class BCDatabase {
public:
    std::unordered_map<std::string, uint32_t> names;
    std::unordered_map<uint32_t, std::unique_ptr<BCBlock>> blocks;
    std::unordered_map<uint32_t, std::unique_ptr<BCLoop>> loops;
    BCTrace trace;

    std::unordered_map<uint64_t, std::vector<uint32_t>> loop_hashes;
    
    std::unordered_map<BCAddr, std::map<BCAddr, int>> next_map;
    std::unordered_map<BCAddr, std::map<BCAddr, int>> prev_map;

    uint64_t base_address;
    uint32_t crc_hash;

    template<typename T, typename... Args>
    BCInsertionResult insert(uint32_t id, const std::string& name, Args&&... args) {

        auto name_it = names.find(name);
        if (name_it != names.end()) {
            // Name already exists
            return { name_it->second, false };
        }

        std::unique_ptr<T> obj = std::make_unique<T>(id, name, std::forward<Args>(args)...);

        if constexpr (std::is_base_of_v<BCBlock, T>) {

            auto block_it = blocks.find(id);

            if (block_it != blocks.end()) {
                // Block already exists
                return { block_it->first, false };
            }

            blocks.emplace(id, std::move(obj));

        } else if constexpr (std::is_same_v<T, BCLoop>) {

            BCLoop* loop = obj.get();

            uint64_t h = hash_window(loop->body.data(), loop->body.size());
            auto loop_it = loop_hashes.find(h);
            if (loop_it != loop_hashes.end()) {
                for (uint32_t match : loop_it->second) {
                    if (loops.at(match)->body == loop->body) {
                        // Loop already exists
                        return { match, false };
                    }
                }
            }

            loop_hashes[h].push_back(id);
            loops.emplace(id, std::move(obj));

        } else {
            static_assert(!sizeof(T), "Unsupported type");
            return { 0, false };
        }

        names[name] = id;
        return { id, true };

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