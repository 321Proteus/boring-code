#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <map>
#include <vector>

#include "address.hpp"
#include "object.hpp"
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
    std::unordered_map<uint32_t, std::unique_ptr<BCBasicBlock>> basic_blocks;
    std::unordered_map<uint32_t, std::unique_ptr<BCBlock>> blocks;
    std::unordered_map<uint32_t, std::unique_ptr<BCLoop>> loops;
    
    BCTrace trace;

    std::unordered_map<BCAddr, uint32_t> basic_blocks_addrs; 
    std::unordered_map<uint64_t, std::vector<uint32_t>> loop_hashes;
    
    std::unordered_map<uint32_t, std::map<uint32_t, int>> next_map;
    std::unordered_map<uint32_t, std::map<uint32_t, int>> prev_map;

    BCAddr base_address;
    uint32_t crc_hash;

    // Todo: get args from tuple and check before creating obj
    template<typename T, typename... Args>
    BCInsertionResult insert(uint32_t id, Args&&... args) {

        // std::string name = obj.get()->name;
        // auto name_it = names.find(name);
        // if (name_it != names.end()) return { name_it->second, false };
        
        if constexpr (std::is_same_v<T, BCBlock>) {

            auto it = blocks.find(id);
            if (it != blocks.end()) return { it->first, false };
            
            std::unique_ptr<T> obj = std::make_unique<T>(id, std::forward<Args>(args)...);
            names[obj.get()->name] = id;
            blocks.emplace(id, std::move(obj));

        } else if constexpr (std::is_same_v<T, BCBasicBlock>) {

            BCAddr addr = std::get<0>(std::forward_as_tuple(args...));
            auto it = basic_blocks_addrs.find(addr);
            if (it != basic_blocks_addrs.end()) return { it->second, false };

            std::unique_ptr<T> obj = std::make_unique<T>(id, std::forward<Args>(args)...);
            names[obj.get()->name] = id;
            basic_blocks.emplace(id, std::move(obj));
            basic_blocks_addrs[addr] = id;

        } else if constexpr (std::is_same_v<T, BCLoop>) {

            auto args_tuple = std::forward_as_tuple(args...);
            auto begin = std::get<0>(args_tuple);
            auto end = std::get<1>(args_tuple);

            uint64_t h = hash_window(begin, end);
            
            auto loop_it = loop_hashes.find(h);
            if (loop_it != loop_hashes.end()) {
                for (uint32_t match : loop_it->second) {
                    if (loops.at(match)->raw_body == loops.at(match)->raw_body) {
                        return { match, false };
                    }
                }
            }

            std::unique_ptr<T> obj = std::make_unique<T>(id, std::forward<Args>(args)...);
            for (uint32_t r : obj.get()->raw_body) {
                obj->body.push_back(resolve_object(r));
            }

            names[obj.get()->name] = id;
            loops.emplace(id, std::move(obj));
            loop_hashes[h].push_back(id);

        } else {

            static_assert(!sizeof(T), "Unsupported type");
            return { 0, false };

        }

        return { id, true };

    }

    BCObject* resolve_object(uint32_t id) const {
        if (id >= LOOP_ID_OFFSET) {
            return loops.at(id - LOOP_ID_OFFSET).get();
        } else if (id >= BLOCK_ID_OFFSET) {
            return blocks.at(id - BLOCK_ID_OFFSET).get();
        } else {
            return basic_blocks.at(id).get();
        }
    }

    bool rename(uint32_t id, std::string newName);

    BCBlock* getBlockByName(const std::string& name) const {
        auto it = names.find(name);
        return (it != names.end()) ? getBlockById(it->second) : nullptr;
    }

    BCBasicBlock* getBasicBlockByName(const std::string& name) const {
        auto it = names.find(name);
        return (it != names.end()) ? getBasicBlockById(it->second) : nullptr;
    }

    BCLoop* getLoopByName(const std::string& name) const {
        auto it = names.find(name);
        return (it != names.end()) ? getLoopById(it->second) : nullptr;
    }

    BCBlock* getBlockById(uint32_t id) const {
        auto it = blocks.find(id);
        return (it != blocks.end()) ? it->second.get() : nullptr;
    }

    BCBasicBlock* getBasicBlockById(uint32_t id) const {
        auto it = basic_blocks.find(id);
        return (it != basic_blocks.end()) ? it->second.get() : nullptr;
    }

    BCLoop* getLoopById(uint32_t id) const {
        auto it = loops.find(id);
        return (it != loops.end()) ? it->second.get() : nullptr;
    }

    void apply_prevs_nexts(BCStatusViewModel& sv);
    void apply_trace(const BCTrace& trace);

    const float HOT_COLD_THRESHOLD = 0.2;

    void find_hot_cold_blocks(BCStatusViewModel& sv);

    BCDatabase() = default;

};