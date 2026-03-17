#include "block.hpp"
#include "database.hpp"
#include "util/util.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

using super = BCBlock;

std::string BCBlock::info(const BCDatabase* db) const {
    std::stringstream in;
    in << '\n';
    in << "Name:     " << name << '\n';
    in << "ID:       " << id << '\n';
    in << '\n';
    in << "Locations (" << loc_count() << "):\n";
    for (auto const& el : locs) 
        in << "   " << to_hex(el) << '\n';
    in << '\n';
    if (prevs.size() == 0) {
        in << "No predecessors\n";
    } else {
        in << prevs.size() << " predecessors: \n";
        for (auto const& [prev, count] : prevs)
            in << "   " << db->getById(prev)->name << " (x" << count << ")\n";
        in << '\n';
    } 

    if (nexts.size() == 0) {
        in << "No successors\n";
    } else {
        in << nexts.size() << " successors:  \n";
        for (auto const& [next, count] : nexts)
            in << "   " << db->getById(next)->name << " (x" << count << ")\n";
        in << '\n';
    } 
    return in.str();
}

std::string BCBasicBlock::info(const BCDatabase* db) const {
    std::string generic = super::info(std::forward<const BCDatabase*>(db));
    return generic + "BCBasicBlock\n";
}
