#include "database.hpp"
#include "core/loader.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include "core/object.hpp"
#include "overload.hpp"
#include "ui/view.hpp"

void BCDatabase::apply_prevs_nexts(BCStatusViewModel& sv) {

    map_successors(*this, flatten(this->trace, false), sv);

    for (auto& [id, obj] : blocks) {
        obj->prevs.clear();
        obj->nexts.clear();
    }
    for (auto& [id, obj] : basic_blocks) {
        obj->prevs.clear();
        obj->nexts.clear();
    }
    for (auto& [id, obj] : loops) {
        obj->prevs.clear();
        obj->nexts.clear();
    }

    size_t progress = 0;

    for (auto& [from_id, targets] : next_map) {

        BCObject* from_obj = resolve_object(from_id);
        if (!from_obj) continue;

        auto& v = from_obj->nexts;
        v.reserve(targets.size());
        for (auto& [to_id, count] : targets) {
            BCObject* to_obj = resolve_object(to_id);
            if (!to_obj) continue;

            v.push_back(Neighbor { to_id, to_obj->name, count });
        }

        progress++;
        sv.update_job_progress(progress);
    }

    for (auto& [to_id, sources] : prev_map) {
        BCObject* to_obj = resolve_object(to_id);
        if (!to_obj) continue;

        auto& vec = to_obj->prevs;
        vec.reserve(sources.size());

        for (auto& [from_id, count] : sources) {
            BCObject* from_obj = resolve_object(from_id);
            if (!from_obj) continue;

            vec.push_back(Neighbor{
                .id = from_id,
                .name = from_obj->name,
                .count = count
            });
        }
    }

    sv.update_job_progress(next_map.size());
}

void BCDatabase::apply_trace(const BCTrace& trace) {
    this->trace = trace;
}

void BCDatabase::find_hot_cold_blocks(BCStatusViewModel& sv) {

    sv.setup_job("Finding hot and cold blocks", trace.size());
    uint64_t progress = 0;

    std::unordered_map<uint32_t, uint32_t> trace_freqs;
    trace_freqs.reserve(blocks.size() + basic_blocks.size() + loops.size());
    for (TraceStep step : trace.steps) {
        std::visit(Overload {
            [&](uint32_t blk_id) {
                trace_freqs[blk_id]++;
            },
            [&](BCLoopInstance const& li) {
                trace_freqs[li.loop_id + LOOP_ID_OFFSET] += li.iterations;
                // for (const BCObject* member : loops[li.loop_id]->body) {
                //     trace_freqs[member->id] += li.iterations;
                // }
            }
        }, step);
        sv.update_job_progress(progress++);
    }

    std::vector<std::pair<uint32_t, uint32_t>> v;
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
        BCObject* obj = resolve_object(id);
        if (!obj) continue;

        double pc = (double)i / (n-1) * 100.0;

        if (i <= cold_k || i >= hot_k) {
            obj->usage_count = { freq, pc };
        } else {
            obj->usage_count = { freq, -1.0 };
        }
    }

}