#include "loader.hpp"
#include <cstdint>
#include <cstdio>
#include <ios>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <cstddef>
#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <vector>
#include "ui/view.hpp"
#include "util/util.hpp"

#include "database.hpp"
#include "block.hpp"
#include "address.hpp"

BCFileType detect_type(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return BCFileType::UNKNOWN;

    char h[4];
    f.read(h, 4);

    if (h[0] == 0x7F && h[1] == 'E' && h[2] == 'L' && h[3] == 'F')
        return BCFileType::ELF;
    if (h[0] == 'M' && h[1] == 'Z')
        return BCFileType::PE;
    if (h[0] == 'B' && h[1] == 'C' && h[3] == 'T')
        return BCFileType::BCTRACE;

    return BCFileType::UNKNOWN;
}

typedef std::vector<std::uint32_t> Trace;

Trace build_chains(BCDatabase& db, const std::vector<BCAddr>& raw, BCStatusViewModel& sv) {

    sv.setup_job("Mapping successors", raw.size());
    for (size_t i=0;i<raw.size()-1;i++) {
        db.next_map[raw[i]][raw[i+1]]++;
        db.prev_map[raw[i+1]][raw[i]]++;
        sv.update_job_progress(i);
    }
    sv.update_job_progress(raw.size());

    sv.setup_job("Scanning for constant successors", db.next_map.size());
    uint64_t cs_progress = 0;

    std::map<BCAddr, BCAddr> glue;
    for (auto const& [src, targets] : db.next_map) {
        if (targets.size() == 1) {
            BCAddr dst = targets.begin()->first;
            if (db.prev_map[dst].size() == 1) glue[src] = dst;
        }
        sv.update_job_progress(cs_progress++);
    }
    sv.update_job_progress(db.next_map.size());

    std::unordered_map<BCAddr, std::string> blk_starts;
    int blk_id = 1;

    std::map<BCAddr, bool> has_incoming_glue;
    for (auto const& [src, dst] : glue) {
        has_incoming_glue[dst] = true;
    }

    sv.setup_job("Generating block definitions", glue.size());

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
        sv.update_job_progress(blk_id);
    }
    sv.update_job_progress(glue.size());

    sv.setup_job("Optimizing trace", raw.size());
    Trace trace;
    
    for (size_t i = 0; i < raw.size(); ) {
        BCAddr curr = raw[i];
        
        sv.update_job_progress(i);

        if (glue.count(curr)) {
            BCBlock* block = db.getByName(blk_starts[curr]);
            trace.push_back(block->get_id());
            i += block->loc_count();
        } else {

            std::string hex_addr = to_hex(curr);

            try {
                db.insert<BCBasicBlock>(blk_id, hex_addr);
                trace.push_back(blk_id);
                blk_id++;
            } catch (std::runtime_error e) {
                BCBlock* to_replace = db.getByName(hex_addr);
                if (to_replace) trace.push_back(to_replace->get_id());
                // printf("Error while inserting block %s (ID %d): %s\n", hex_addr.c_str(), blk_id, e.what());
            }
            i++;
        }
        sv.update_job_progress(i);
    }

    sv.update_job_progress(raw.size());
    
    return trace;
}

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv) {

    std::vector<BCAddr> raw_data;
    std::ifstream f(path, std::ios::binary);

    f.seekg(0, std::ios::end);
    uint64_t size = f.tellg();
    f.seekg(4, std::ios::beg);

    uint8_t version; f.read(reinterpret_cast<char*>(&version), 1);
    bool is_x64; f.read(reinterpret_cast<char*>(&is_x64), 1);
    uint32_t hash; f.read(reinterpret_cast<char*>(&hash), 4);
    uint64_t base; f.read(reinterpret_cast<char*>(&base), 8);

    
    sv.setup_job("Loading trace", size / (is_x64 ? 8 : 4));
    int read = 0;

    if (is_x64) {
        uint64_t a;
        while (f.read((char*)&a, sizeof(uint64_t))) {
            raw_data.push_back(a - base);
            sv.update_job_progress(read++);
        }
    } else {
        uint32_t a;
        while (f.read((char*)&a, sizeof(uint32_t))) {
            raw_data.push_back(a - base);
            sv.update_job_progress(read++);
        }
    }

    BCDatabase db;
    
    Trace t1 = build_chains(db, raw_data, sv);

    printf("Architecture: %s \nHash: %04X \nBase: %08lX", (is_x64 ? "x64" : "x86"), hash, base);

    std::cout
        << "\nAnalysis done with " << db.blocks.size() << " unique addrs and "
        << t1.size() << " lines of trace. Saving..." << std::endl;

    db.apply_trace(t1);
    db.apply_prevs_nexts();

    db.find_hot_cold_blocks();

    std::ofstream out("final_timeline.txt");
    for (const auto& s : t1) out << s << "\n";

    std::ofstream dict_out("structure_dictionary.txt");
    for (auto const& [id, block] : db.blocks) {
        dict_out << block->name << ": ";
        for (const auto& c : block->locs) dict_out << to_hex(c) << " ";
        dict_out << "\n";
    }

    std::cout << "Output trace saved." << std::endl;
    return db;
}
