#include "database.hpp"
#include "core/address.hpp"
#include "loader.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include "object.hpp"
#include "overload.hpp"
#include "view.hpp"

BCBasicBlock* BCObjectStore::get_bbl(BCObjectId id) const {
    uint32_t i = id.index();
    return (i < basic_blocks_.size() ? basic_blocks_[i].get() : nullptr);
}

BCBasicBlock* BCObjectStore::get_bbl(uint32_t index) const {
    return (index < basic_blocks_.size() ? basic_blocks_[index].get() : nullptr);
}

BCBlock* BCObjectStore::get_block(BCObjectId id) const {
    uint32_t i = id.index();
    return (i < blocks_.size() ? blocks_[i].get() : nullptr);
}

BCBlock* BCObjectStore::get_block(uint32_t index) const {
    return (index < blocks_.size() ? blocks_[index].get() : nullptr);
}


BCLoop* BCObjectStore::get_loop(BCObjectId id) const {
    uint32_t i = id.index();
    return (i < loops_.size() ? loops_[i].get() : nullptr);
}

BCLoop* BCObjectStore::get_loop(uint32_t index) const {
    return (index < loops_.size() ? loops_[index].get() : nullptr);
}

BCObject* BCObjectStore::get(BCObjectId id) const {
    switch (id.type()) {
        case BCObjectType::BasicBlock:
            return get_bbl(id); break;
        case BCObjectType::Block:
            return get_block(id); break;
        case BCObjectType::Loop:
            return get_loop(id); break;
        default:
            return nullptr; break;
    }
}

BCObject* BCObjectStore::get_by_name(const std::string& name) const {
    auto it = names_.find(name);
    return (it != names_.end() ? get(it->second) : nullptr);
}

BCBasicBlock* BCObjectStore::get_by_addr(BCAddr address) const {
    auto it = basic_blocks_addrs_.find(address);
    return (it != basic_blocks_addrs_.end() ? basic_blocks_.at(it->second).get() : nullptr );
}

BCInsertionResult BCObjectStore::insert_basic_block(BCAddr address) {

    auto it = basic_blocks_addrs_.find(address);
    if (it != basic_blocks_addrs_.end()) return { { it->second, BCObjectType::BasicBlock }, false };

    BCObjectId id = { next_bbl_id_++, BCObjectType::BasicBlock };

    std::unique_ptr<BCBasicBlock> obj = std::make_unique<BCBasicBlock>(id, address);
    names_[obj.get()->name] = id;
    basic_blocks_.push_back(std::move(obj));
    basic_blocks_addrs_[address] = id.index();

    return { id, true };

}

BCInsertionResult BCObjectStore::insert_block(std::vector<BCObject*> members) {

    BCObjectId id = { next_block_id_++, BCObjectType::Block };

    std::unique_ptr<BCBlock> obj = std::make_unique<BCBlock>(id, members);
    names_[obj.get()->name] = id;
    blocks_.push_back(std::move(obj));
    
    return { id, true };

}

void BCDatabase::apply_prevs_nexts(BCStatusViewModel& sv) {

    map_successors(*this, flatten(this->trace, false), sv);

    for (auto& obj : store.blocks()) {
        obj->prevs.clear();
        obj->nexts.clear();
    }
    for (auto& obj : store.basic_blocks()) {
        obj->prevs.clear();
        obj->nexts.clear();
    }
    for (auto& obj : store.loops()) {
        obj->prevs.clear();
        obj->nexts.clear();
    }

    size_t progress = 0;

    for (auto& [from_id, targets] : next_map) {

        BCObject* from_obj = store.get(from_id);
        if (!from_obj) continue;

        auto& v = from_obj->nexts;
        v.reserve(targets.size());
        for (auto& [to_id, count] : targets) {
            BCObject* to_obj = store.get(to_id);
            if (!to_obj) continue;

            v.push_back(Neighbor { to_id, to_obj->name, count });
        }

        progress++;
        sv.update_job_progress(progress);
    }

    for (auto& [to_id, sources] : prev_map) {
        BCObject* to_obj = store.get(to_id);
        if (!to_obj) continue;

        auto& vec = to_obj->prevs;
        vec.reserve(sources.size());

        for (auto& [from_id, count] : sources) {
            BCObject* from_obj = store.get(from_id);
            if (!from_obj) continue;

            vec.push_back(Neighbor{ from_id, from_obj->name, count });
        }
    }

    sv.update_job_progress(next_map.size());
}

void BCDatabase::apply_trace(const BCTrace& trace) {
    this->trace = std::move(trace);
}

void BCDatabase::find_hot_cold_blocks(BCStatusViewModel& sv) {

    sv.setup_job("Finding hot and cold blocks", trace.size());
    uint64_t progress = 0;

    std::unordered_map<BCObjectId, uint32_t> trace_freqs;
    trace_freqs.reserve(store.basic_blocks().size() + store.blocks().size() + store.loops().size());
    for (TraceStep step : trace.steps) {
        std::visit(Overload {
            [&](BCObjectId blk_id) {
                trace_freqs[blk_id]++;
            },
            [&](BCLoopInstance const& li) {
                trace_freqs[li.full_id()] += li.iterations;
                // for (const BCObject* member : loops[li.loop_id]->body) {
                //     trace_freqs[member->id] += li.iterations;
                // }
            }
        }, step);
        sv.update_job_progress(progress++);
    }

    std::vector<std::pair<BCObjectId, uint32_t>> v;
    v.reserve(trace_freqs.size());

    for (auto const& [id, count] : trace_freqs)
        v.emplace_back(id, count);

    std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    });

    const int n = v.size();
    if (n == 0) return;

    size_t hot_k = static_cast<size_t>((1.0 - HOT_COLD_THRESHOLD) * n);
    size_t cold_k = static_cast<size_t>(HOT_COLD_THRESHOLD * n);

    for (int i=0;i<n;i++) {
        auto [id, freq] = v[i];
        BCObject* obj = store.get(id);
        if (!obj) continue;

        double pc = (double)i / (n-1) * 100.0;

        if (i <= cold_k || i >= hot_k) {
            obj->usage_count = { freq, pc };
        } else {
            obj->usage_count = { freq, -1.0 };
        }
    }

}