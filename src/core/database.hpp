#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>

#include "address.hpp"
#include "object.hpp"
#include "loader.hpp"
#include "view.hpp"
#include "util/hash.h"

class BCBlock;

struct BCInsertionResult {
    BCObjectId id;
    bool created;
};

class BCObjectStore {
public:

    BCBlock*            get_block(BCObjectId id) const;
    BCBasicBlock*       get_bbl(BCObjectId id) const;
    BCLoop*             get_loop(BCObjectId id) const;

    BCBlock*            get_block(uint32_t index) const;
    BCBasicBlock*       get_bbl(uint32_t index) const;
    BCLoop*             get_loop(uint32_t index) const;

    BCObject*           get(BCObjectId id) const;
    BCBasicBlock*       get_by_addr(BCAddr address) const;
    BCObject*           get_by_name(const std::string& name) const;

    BCInsertionResult   insert_basic_block(BCAddr addr);
    BCInsertionResult   insert_block(const std::vector<BCObject*> members);

    template <typename Iterator>
    BCInsertionResult insert_loop(Iterator begin, Iterator end) {

        uint64_t h = hash_window(begin, end);
        
        auto loop_it = loop_hashes_.find(h);
        if (loop_it != loop_hashes_.end()) {
            for (uint32_t match : loop_it->second) {
                const auto& existing = loops_.at(match);
                if (std::equal(begin, end, existing->raw_body.begin(), existing->raw_body.end())) {
                    return { { match, BCObjectType::Loop }, false };
                }
            }
        }

        BCObjectId id = { next_loop_id_++, BCObjectType::Loop };

        std::unique_ptr<BCLoop> obj = std::make_unique<BCLoop>(id, begin, end);
        for (BCObjectId r : obj.get()->raw_body) {
            obj->body.push_back(get(r));
        }

        names_[obj.get()->name] = id;
        loops_.push_back(std::move(obj));
        loop_hashes_[h].push_back(id.index());

        return { id, true };

    }

    auto const& basic_blocks() const { return basic_blocks_; };
    auto const& blocks() const { return blocks_; };
    auto const& loops() const { return loops_; };

private:
    std::vector<std::unique_ptr<BCBasicBlock>> basic_blocks_;
    std::vector<std::unique_ptr<BCBlock>>      blocks_;
    std::vector<std::unique_ptr<BCLoop>>       loops_;

    std::unordered_map<BCAddr, uint32_t>                        basic_blocks_addrs_;
    std::unordered_map<std::string, BCObjectId>                 names_;
    std::unordered_map<uint64_t, std::vector<uint32_t>>         loop_hashes_;

    uint32_t next_bbl_id_   = 0;
    uint32_t next_block_id_ = 0;
    uint32_t next_loop_id_  = 0;
};

class BCDatabase {
public:
    
    BCObjectStore store;
    
    BCTrace trace;
    
    std::unordered_map<BCObjectId, std::map<BCObjectId, int>> next_map;
    std::unordered_map<BCObjectId, std::map<BCObjectId, int>> prev_map;

    BCAddr base_address;
    uint32_t crc_hash;

    void apply_prevs_nexts(BCStatusViewModel& sv);
    void apply_trace(const BCTrace& trace);

    const float HOT_COLD_THRESHOLD = 0.2;

    void find_hot_cold_blocks(BCStatusViewModel& sv);

    BCDatabase() = default;

};