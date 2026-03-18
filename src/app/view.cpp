#include "view.hpp"
#include "core/block.hpp"
#include "core/util/util.hpp"
#include <cstdint>
#include <iostream>
#include <sstream>

void QtViewModel::show_details(const BCBlock::Details& details) const {

    std::stringstream iss;
    iss << '\n';
    iss << "Name:     " << details.name << '\n';
    iss << "ID:       " << details.id << '\n';
    iss << '\n';
    iss << "Locations (" << details.locs.size() << "):\n";

    for (auto const& el : details.locs) 
        iss << "   " << to_hex(el) << '\n';
    iss << '\n';
    
    if (details.prevs.size() == 0) {
        iss << "Predecessors: None\n";
    } else {
        iss << details.prevs.size() << " predecessors: \n";
        for (const Neighbor& prev : details.prevs)
            iss << "   " << prev.name << " (x" << prev.count << ")\n";
        iss << '\n';
    } 

    if (details.nexts.size() == 0) {
        iss << "Successors: None\n";
    } else {
        iss << details.nexts.size() << " successors: \n";
        for (const Neighbor& next : details.nexts)
            iss << "   " << next.name << " (x" << next.count << ")\n";
        iss << '\n';
    }

    ui.details_text->setText(QString::fromStdString(iss.str()));

}

void QtViewModel::show_details(const BCBasicBlock& details) const {

}

void QtViewModel::show_trace() const {
    
}

void QtViewModel::setup_job(uint64_t size) {
    job_progress = { 0, size };
}

void QtViewModel::update_job_progress(uint64_t new_progress) {
    uint64_t total = job_progress.second;
    job_progress.first = new_progress;
    if (new_progress % (total/1000) == 0 || new_progress == total) printf("\rProgress: %lu/%lu (%.1f%%)", new_progress, total, ((float)new_progress/total*100));

}

void QtViewModel::show_error(const std::string& msg) const {

}
