#include "view.hpp"
#include "core/object.hpp"
#include "core/view.hpp"
#include <iostream>

void display_object_info(const BCObject& object) {

    std::cout << '\n';
    std::cout << "Name: \t\t" << object.name << '\n';
    std::cout << "ID: \t\t" << object.id << '\n';
    std::cout << "Usage count: \t" << object.usage_count.value << '\n';
    std::cout << '\n';

    if (object.prevs.size() == 0) {
        std::cout << "Predecessors: None\n";
    } else {
        std::cout << object.prevs.size() << " predecessors: \n";
        for (const Neighbor& prev : object.prevs)
            std::cout << "   " << prev.name << " (x" << prev.count << ")\n";
        std::cout << '\n';
    } 

    if (object.nexts.size() == 0) {
        std::cout << "Successors: None\n";
    } else {
        std::cout << object.nexts.size() << " successors: \n";
        for (const Neighbor& next : object.nexts)
            std::cout << "   " << next.name << " (x" << next.count << ")\n";
        std::cout << '\n';
    }
}

void ConsoleViewModel::show_details(const BCBlock& block) const {

    display_object_info(block);

    std::cout << "Locations (" << block.members.size() << "):\n";

    for (const BCObject* m : block.members) 
        std::cout << '\t' << m->name << '\n';

}

void ConsoleViewModel::show_details(const BCBasicBlock& bb) const {
    display_object_info(bb);
}

void ConsoleViewModel::show_details(const BCLoop& loop) const {
    display_object_info(loop);
    
    std::cout << "Members (" << loop.body.size() << "):\n";
    for (const BCObject* el : loop.body)
        std::cout << '\t' << el->name << '\n';
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
