#include "view.hpp"
#include "core/block.hpp"
#include "core/util/util.hpp"
#include <iostream>

void ConsoleViewModel::show_details(const BCBlock::Details& details) {

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

void ConsoleViewModel::show_details(const BCBasicBlock& details) {

}

void ConsoleViewModel::show_trace() const {
    
}

void ConsoleViewModel::setup_job(const std::string name, uint64_t size) {
    job = { name, size, 0 };
    std::cout << std::endl << name << "..." << std::endl;
}

void ConsoleViewModel::update_job_progress(uint64_t progress) {
    job.progress = progress;
    uint64_t size = job.size;
    if (progress % (size/1000) == 0 || progress == size) {
        printf("\rProgress: %lu/%lu (%.1f%%)", progress, size, (float)progress/size*100);
    }

}

void ConsoleViewModel::show_error(const std::string& msg) const {
    std::cerr << "Error: " << msg << std::endl;
}
