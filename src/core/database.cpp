#include "database.hpp"
#include "block.hpp"
#include "address.hpp"
#include <cstdint>

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

void BCDatabase::apply_trace(const std::vector<uint32_t>& trace) {
    this->trace = trace;
}

void BCDatabase::setup_job(uint64_t size) {
    job_progress = { 0, size };
}

void BCDatabase::update_job_progress(uint64_t new_progress) {

    uint64_t total = job_progress.second;
    job_progress.first = new_progress;
    if (new_progress % (total/1000) == 0 || new_progress == total) printf("\rProgress: %lu/%lu (%.1f%%)", new_progress, total, ((float)new_progress/total*100));
}

BCBlock* BCDatabase::getByLoc(BCAddr address) const {
    for (auto const& [id, block] : blocks) {
        if (block->check_loc(address)) return block.get();
    }
    return nullptr;
}