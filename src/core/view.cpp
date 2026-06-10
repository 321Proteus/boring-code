#include "view.hpp"
#include "core/object.hpp"
#include "database.hpp"
#include <memory>
#include <vector>

std::shared_ptr<std::vector<BCTraceEntry>> BCTraceViewModel::precompute_trace(const BCDatabase& db, BCStatusViewModel& sv) {
    const auto& tr = db.trace;
    uint64_t size = tr.size();
    
    auto trace_list = std::make_shared<std::vector<BCTraceEntry>>();
    trace_list->reserve(size);

    sv.setup_job("Precomputing UI trace entries", size);
    for (uint64_t i=0;i<size;i++) {
        TraceStep step = tr.steps[i];
        std::visit(Overload {
            [&](BCObjectId id) {
                trace_list->push_back({ id, db.store.get(id)->name });
            },
            [&](BCLoopInstance li) {
                BCLoop* loop = db.store.get_loop(li.loop_id);
                trace_list->push_back({ li.loop_id, loop->name + " (x" + std::to_string(li.iterations) + ")" });
            }
        }, step);
        sv.update_job_progress(i);
    }
    sv.update_job_progress(size);
    return trace_list;
}
