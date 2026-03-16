#include "database.hpp"
#include "block.hpp"
#include "address.hpp"

void BCDatabase::apply_prevs_nexts() {
    for (auto const& [id, block] : blocks) {

        BCAddr first = block->first_loc();
        BCAddr last = block->last_loc();

        std::map<BCAddr, int> nexts = next_map[last];
        std::map<BCAddr, int> prevs = prev_map[first];

        for (auto const& [next, count] : nexts) {
            for (auto const& [other_id, other_block] : blocks) {
                if (next == other_block->first_loc() && id != other_id) {
                    block->nexts[other_id] = count;
                }
            }
        }

        for (auto const& [prev, count] : prevs) {
            for (auto const& [other_id, other_block] : blocks) {
                if (prev == other_block->first_loc() && id != other_id) {
                    block->prevs[other_id] = count;
                }
            }
        }
    }
}

void BCDatabase::apply_trace(const std::vector<std::string>& trace) {
    this->trace = std::vector<uint32_t>();
    this->trace.reserve(trace.size());
    for (const std::string& step : trace) {
        this->trace.push_back(names[step]);
    }
}

BCBlock* BCDatabase::getByLoc(BCAddr address) const {
    for (auto const& [id, block] : blocks) {
        if (block->check_loc(address)) return block.get();
    }
    return nullptr;
}