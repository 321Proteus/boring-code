#include "block.hpp"
#include <iostream>

using std::cout;
using super = BCBlock;

void BCBlock::info() const {
    cout << '\n';
    cout << "Name:     " << name << '\n';
    cout << "ID:       " << id << '\n';
    cout << '\n';
    cout << "Locations (" << loc_count() << "):\n";
    for (auto const& el : locs) 
        cout << "   " << el << '\n';
    cout << '\n';
    if (prevs.size() == 0) {
        std::cout << "No predecessors\n";
    } else {
        std::cout << prevs.size() << " predecessors: \n";
        for (auto const& [next, count] : prevs)
            std::cout << "   " << next << " (x" << count << ")\n";
        std::cout << '\n';
    } 

    if (nexts.size() == 0) {
        std::cout << "No successors\n";
    } else {
        std::cout << nexts.size() << " successors:  \n";
        for (auto const& [next, count] : nexts)
            std::cout << "   " << next << " (x" << count << ")\n";
        std::cout << '\n';
    } 

}

void BCBasicBlock::info() const {
    super::info();
    cout << "BCBasicBlock\n";
}