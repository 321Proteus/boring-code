#include "database.hpp"
#include "block.hpp"
#include "address.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

void BCDatabase::apply_prevs_nexts() {

    std::unordered_map<BCAddr, int> starts;
    for (auto const& [id, block] : blocks) starts[block->first_loc()] = id;

    for (auto const& [id, block] : blocks) {

        BCAddr first = block->first_loc();
        BCAddr last = block->last_loc();

        if (next_map.count(last)) {
            for (auto const& [next_addr, count] : next_map[last]) {
                auto it = starts.find(next_addr);
                if (it != starts.end() && it->second != id) {
                    block->nexts[it->second] = count;
                }
            }
        }
        
        if (prev_map.count(first)) {
            for (auto const& [prev_addr, count] : prev_map[first]) {
                auto it = starts.find(prev_addr);
                if (it != starts.end() && it->second != id) {
                    block->prevs[it->second] = count;
                }
            }
        }
    }
}

void BCDatabase::apply_trace(const std::vector<uint32_t>& trace) {
    this->trace = trace;
}

BCBlock* BCDatabase::getByLoc(BCAddr address) const {
    for (auto const& [id, block] : blocks) {
        if (block->check_loc(address)) return block.get();
    }
    return nullptr;
}

void BCDatabase::find_hot_cold_blocks() {

    std::unordered_map<uint32_t, uint32_t> trace_freqs;
    trace_freqs.reserve(blocks.size());

    for (uint32_t blk_id : trace) trace_freqs[blk_id]++;

    std::vector<std::pair<uint32_t, uint32_t>> v;
    v.reserve(trace_freqs.size());

    for (auto const& [id, count] : trace_freqs)
        v.emplace_back(id, count);

    size_t hot_k = static_cast<size_t>((1.0 - HOT_COLD_THRESHOLD) * v.size());
    size_t cold_k = static_cast<size_t>(HOT_COLD_THRESHOLD * v.size());

    std::nth_element(
        v.begin(),
        v.begin() + hot_k,
        v.end(),
        [](auto const& a, auto const& b) { return a.second < b.second; }
    );
    std::vector<std::pair<uint32_t, uint32_t>> hot(v.begin() + hot_k, v.end());

    std::nth_element(
        v.begin(),
        v.begin() + cold_k,
        v.end(),
        [](auto const& a, auto const& b) { return a.second < b.second; }
    );
    std::vector<std::pair<uint32_t, uint32_t>> cold(v.begin(), v.begin() + cold_k);
    
    uint32_t min_hot = std::min_element(hot.begin(), hot.end(),
    [](const auto& a, const auto& b) {
        return a.second < b.second;
    })->second;

    uint32_t max_hot = std::max_element(hot.begin(), hot.end(),
    [](const auto& a, const auto& b) {
        return a.second < b.second;
    })->second;

    uint32_t min_cold = std::min_element(cold.begin(), cold.end(),
    [](const auto& a, const auto& b) {
        return a.second < b.second;
    })->second;

    uint32_t max_cold = std::max_element(cold.begin(), cold.end(),
    [](const auto& a, const auto& b) {
        return a.second < b.second;
    })->second;

    for (auto const& [id, block] : blocks) {
        uint32_t freq = trace_freqs[id];
        if (freq >= min_hot || freq <= max_cold) {
            block->usage_count = { freq, (float)freq / max_hot * 100 };
        } else {
            block->usage_count = { freq, -1 };
        }
    }

    // std::sort(hot.begin(), hot.end(), [](const auto& a, const auto& b) {
    //     return a.second > b.second;
    // });
    // std::sort(cold.begin(), cold.end(), [](const auto& a, const auto& b) {
    //     return a.second < b.second;
    // });
    
    // std::cout << "Hot\n";
    // for (int i=0;i<10;i++) {
    //     std::cout << blocks[hot[i].first]->name << ' ' << hot[i].second << std::endl;
    // }

    // std::cout << "Cold\n";
    // for (int i=0;i<10;i++) {
    //     std::cout << blocks[cold[i].first]->name << ' ' << cold[i].second << std::endl;
    // }


}

BCBlock::Details BCDatabase::generate_details(const BCBlock& block) const {

    BCBlock::Details d = {
        .id=block.get_id(),
        .name=block.name,
        .usage_count=block.usage_count,
        .locs=block.locs, 
        .prevs={},
        .nexts={}
    };

    for (const auto& [prev, count] : block.prevs)
        d.prevs.push_back({ prev, getById(prev)->name, count });

    for (const auto& [next, count] : block.nexts)
        d.nexts.push_back({ next, getById(next)->name, count });

    return d;

}