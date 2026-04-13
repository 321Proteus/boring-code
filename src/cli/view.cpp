#include "view.hpp"
#include "core/object.hpp"
#include "ui/view.hpp"
#include <iostream>

void ConsoleViewModel::show_details(const BCObject& object) {

    std::cout << '\n';
    std::cout << "Name: \t\t" << object.name << '\n';
    std::cout << "ID: \t\t" << object.id << '\n';
    std::cout << "Usage count: \t" << object.usage_count.value << '\n';
    std::cout << '\n';
    // std::cout << "Locations (" << details.members.size() << "):\n";

    // for (auto const& el : details.members) 
    //     std::cout << "   " << to_hex(el) << '\n';
    // std::cout << '\n';
    
    // if (details.prevs.size() == 0) {
    //     std::cout << "Predecessors: None\n";
    // } else {
    //     std::cout << details.prevs.size() << " predecessors: \n";
    //     for (const Neighbor& prev : details.prevs)
    //         std::cout << "   " << prev.name << " (x" << prev.count << ")\n";
    //     std::cout << '\n';
    // } 

    // if (details.nexts.size() == 0) {
    //     std::cout << "Successors: None\n";
    // } else {
    //     std::cout << details.nexts.size() << " successors: \n";
    //     for (const Neighbor& next : details.nexts)
    //         std::cout << "   " << next.name << " (x" << next.count << ")\n";
    //     std::cout << '\n';
    // }

}

void ConsoleViewModel::show_trace(std::shared_ptr<std::vector<BCTraceEntry>> trace) const {
    
}

void ConsoleViewModel::setup_job(const std::string name, uint64_t size) {
    job = { name, size, 0 };
    std::cout << std::endl << name << "..." << std::endl;
}

void ConsoleViewModel::update_job_progress(uint64_t progress) {
    uint64_t size = job.size;
    if (progress % (size/1000) == 0 || progress == size) {
        printf("\rProgress: %lu/%lu (%.1f%%)", progress, size, (float)progress/size*100);
    }

}

void ConsoleViewModel::show_error(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
}
