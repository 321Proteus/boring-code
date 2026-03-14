#include <cstdio>
#include <stdexcept>
#include <unordered_map>
#include <cstddef>
#include <iostream>
#include <map>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>

#include "database.hpp"
#include "block.hpp"
#include "address.hpp"

BCDatabase db;

typedef std::vector<std::string> Trace;

std::map<std::string, std::unique_ptr<BCBlock>> dictionary;

std::string to_hex(BCAddr val) {
    std::stringstream ss;
    ss << std::hex << val;
    return ss.str();
}

Trace build_chains(
    const std::vector<BCAddr>& raw,
    std::map<BCAddr, std::map<BCAddr, int>>& next_map,
    std::map<BCAddr, std::map<BCAddr, int>>& prev_map
) {
    for (size_t i=0;i<raw.size()-1;i++) {
        next_map[raw[i]][raw[i+1]]++;
        prev_map[raw[i+1]][raw[i]]++;
    }

    std::map<BCAddr, BCAddr> glue;
    for (auto const& [src, targets] : next_map) {
        if (targets.size() == 1) {
            BCAddr dst = targets.begin()->first;
            if (prev_map[dst].size() == 1) glue[src] = dst;
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
            std::vector<std::string> members;
            BCAddr curr = src;

            while (glue.count(curr)) {
                members.push_back(to_hex(curr));
                curr = glue[curr];
            }
            members.push_back(to_hex(curr));

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

std::vector<BCBlock> load_blocks() {

    std::vector<BCAddr> raw_data;
    std::ifstream f("functions.bin", std::ios::binary);
    BCAddr a; while(f.read((char*)&a, sizeof(BCAddr))) raw_data.push_back(a);
    std::cout << "Start: " << raw_data.size() << " addrs\n";

    std::map<BCAddr, std::map<BCAddr, int>> next_map, prev_map;
    
    Trace t1 = build_chains(raw_data, next_map, prev_map);

    std::cout
        << "Analysis done with " << db.blocks.size() << " unique addrs and "
        << t1.size() << " lines of trace. Saving...\n";

    for (auto const& [id, block] : db.blocks) {

        BCAddr first = block->first_loc();
        BCAddr last = block->last_loc();

        std::map<BCAddr, int> nexts = next_map[last];
        std::map<BCAddr, int> prevs = prev_map[first];

        for (auto const& [next, count] : nexts) {
            for (auto const& [other_id, other_block] : db.blocks) {
                if (next == other_block->first_loc() && id != other_id) {
                    block->nexts[other_id] = count;
                }
            }
        }

        for (auto const& [prev, count] : prevs) {
            for (auto const& [other_id, other_block] : db.blocks) {
                if (prev == other_block->first_loc() && id != other_id) {
                    block->prevs[other_id] = count;
                }
            }
        }
        
        // printf("Found %zu prevs and %zu nexts for ID %d\n", block->prevs.size(), block->nexts.size(), id);
        
    }

    std::ofstream out("final_timeline.txt");
    for (const auto& s : t1) out << s << "\n";

    std::ofstream dict_out("structure_dictionary.txt");
    for (auto const& [id, block] : db.blocks) {
        dict_out << block->name << ": ";
        for (const auto& c : block->locs) dict_out << c << " ";
        dict_out << "\n";
    }

    std::cout << "Output trace saved.\n";

    while (true) {
        std::cout << "Enter a block name to search: ";
        std::string block_name; std::cin >> block_name;
        BCBlock* block = db.getByName(block_name);

        if (!block) {
            std::cout << "Block not found!\n";
            continue;
        }

        block->info();

    }

    return {};
}

int main(int argc, char* argv[]) {
    load_blocks();
}
