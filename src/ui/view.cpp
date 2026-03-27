#include "view.hpp"
#include "core/database.hpp"

std::shared_ptr<std::vector<BCTraceEntry>> BCTraceViewModel::precompute_trace(const BCDatabase& db, BCStatusViewModel& sv) {
    const auto& tr = db.trace;
    uint64_t size = tr.size();
    
    auto trace_list = std::make_shared<std::vector<BCTraceEntry>>();
    trace_list->reserve(size);

    sv.setup_job("Precomputing UI trace entries", size);
    for (uint64_t i=0;i<size;i++) {
        uint32_t id = tr[i];
        trace_list->push_back({ db.getById(id)->name, id });
        sv.update_job_progress(i);
    }
    sv.update_job_progress(size);
    return trace_list;
}