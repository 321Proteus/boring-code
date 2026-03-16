#include <cstdio>
#include <stdexcept>
#include <unordered_map>
#include <cstddef>
#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <vector>
#include "util/util.hpp"

#include "database.hpp"
#include "block.hpp"
#include "address.hpp"

typedef std::vector<std::string> Trace;

Trace build_chains(BCDatabase& db, const std::vector<BCAddr>& raw) {

    for (size_t i=0;i<raw.size()-1;i++) {
        db.next_map[raw[i]][raw[i+1]]++;
        db.prev_map[raw[i+1]][raw[i]]++;
    }

    std::map<BCAddr, BCAddr> glue;
    for (auto const& [src, targets] : db.next_map) {
        if (targets.size() == 1) {
            BCAddr dst = targets.begin()->first;
            if (db.prev_map[dst].size() == 1) glue[src] = dst;
        }
    }

    std::unordered_map<BCAddr, std::string> blk_starts;
    int blk_id = 1;

    std::map<BCAddr, bool> has_incoming_glue;
    for (auto const& [src, dst] : glue) {
        has_incoming_glue[dst] = true;
    }

    for (auto const& [src, dst] : glue) {
        if (!has_incoming_glue[src]) {
            std::string name = "BLK_" + std::to_string(blk_id);
            std::vector<BCAddr> members;
            BCAddr curr = src;

            while (glue.count(curr)) {
                members.push_back(curr);
                curr = glue[curr];
            }
            members.push_back(curr);

            blk_starts[src] = name;

            try {
                db.insert<BCBlock>(blk_id, name, members);
                blk_id++;
            } catch (std::runtime_error e) {
                printf("Error while inserting block %s (ID %d): %s\n", name.c_str(), blk_id, e.what());
            }

        }
    }

    Trace trace;
    
    for (size_t i = 0; i < raw.size(); ) {
        BCAddr curr = raw[i];

        if (glue.count(curr)) {
            BCBlock* block = db.getByName(blk_starts[curr]);
            trace.push_back(block->name);
            i += block->loc_count();
        } else {

            std::string hex_addr = to_hex(curr);

            try {
                db.insert<BCBasicBlock>(blk_id, hex_addr);
                blk_id++;
            } catch (std::runtime_error e) {
                // printf("Error while inserting block %s (ID %d): %s\n", hex_addr.c_str(), blk_id, e.what());
            }

            trace.push_back(hex_addr);
            i++;

        }
    }
    return trace;
}

BCDatabase load_database(const std::string& path) {

    std::vector<BCAddr> raw_data;
    std::ifstream f(path, std::ios::binary);
    BCAddr a; while(f.read((char*)&a, sizeof(BCAddr))) raw_data.push_back(a);
    std::cout << "Start: " << raw_data.size() << " addrs" << std::endl;

    BCDatabase db;
    
    Trace t1 = build_chains(db, raw_data);

    std::cout
        << "Analysis done with " << db.blocks.size() << " unique addrs and "
        << t1.size() << " lines of trace. Saving..." << std::endl;

    db.apply_trace(t1);
    db.apply_prevs_nexts();

    std::ofstream out("final_timeline.txt");
    for (const auto& s : t1) out << s << "\n";

    std::ofstream dict_out("structure_dictionary.txt");
    for (auto const& [id, block] : db.blocks) {
        dict_out << block->name << ": ";
        for (const auto& c : block->locs) dict_out << c << " ";
        dict_out << "\n";
    }

    std::cout << "Output trace saved." << std::endl;

    return db;

}
