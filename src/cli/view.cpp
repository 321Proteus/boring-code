#include "view.hpp"
#include "core/block.hpp"
#include "core/util/util.hpp"
#include <iostream>

void BCConsoleView::show_details(const BCBlock::Details& details) const {

    std::cout << '\n';
    std::cout << "Name:     " << details.name << '\n';
    std::cout << "ID:       " << details.id << '\n';
    std::cout << '\n';
    std::cout << "Locations (" << details.locs.size() << "):\n";

    for (auto const& el : details.locs) 
        std::cout << "   " << to_hex(el) << '\n';
    std::cout << '\n';
    
    if (details.prevs.size() == 0) {
        std::cout << "Predecessors: None\n";
    } else {
        std::cout << details.prevs.size() << " predecessors: \n";
        for (const Neighbor& prev : details.prevs)
            std::cout << "   " << prev.name << " (x" << prev.count << ")\n";
        std::cout << '\n';
    } 

    if (details.nexts.size() == 0) {
        std::cout << "Successors: None\n";
    } else {
        std::cout << details.nexts.size() << " successors: \n";
        for (const Neighbor& next : details.nexts)
            std::cout << "   " << next.name << " (x" << next.count << ")\n";
        std::cout << '\n';
    }

}

void BCConsoleView::show_details(const BCBasicBlock& details) const {

}

void BCConsoleView::show_trace() const {
    
}

void BCConsoleView::setup_job(uint64_t size) {
    job_progress = { 0, size };
}

void BCConsoleView::update_job_progress(uint64_t new_progress) {
    uint64_t total = job_progress.second;
    job_progress.first = new_progress;
    if (new_progress % (total/1000) == 0 || new_progress == total) printf("\rProgress: %lu/%lu (%.1f%%)", new_progress, total, ((float)new_progress/total*100));
}

void BCConsoleView::show_error(const std::string& msg) const {
    std::cerr << "Error: " << msg << std::endl;
}