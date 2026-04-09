#include "loader.hpp"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ios>
#include <ostream>
#include <unordered_map>
#include <cstddef>
#include <iostream>
#include <map>
#include <fstream>
#include <string>
#include <variant>
#include <vector>
#include "ui/view.hpp"
#include "overload.hpp"

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

BCTrace build_chains(BCDatabase& db, const std::vector<BCAddr>& raw, BCStatusViewModel& sv) {
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
            // if (src == dst) continue;
            if (db.prev_map[dst].size() == 1) glue[src] = dst;
        }
        sv.update_job_progress(cs_progress++);
    }
    sv.update_job_progress(db.next_map.size());

    std::unordered_map<BCAddr, uint32_t> blk_starts;
    uint32_t blk_id = 1;

    std::map<BCAddr, bool> has_incoming_glue;
    for (auto const& [src, dst] : glue) {
        has_incoming_glue[dst] = true;
    }

    sv.setup_job("Generating block definitions", glue.size());
    printf("Sizing: %lu, %lu\n", glue.size(), has_incoming_glue.size());

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

            blk_starts[src] = blk_id;

            db.insert<BCBlock>(blk_id, name, members);
            blk_id++;

        }
        sv.update_job_progress(blk_id);
    }

    sv.update_job_progress(glue.size());
    sv.setup_job("Optimizing trace", raw.size());
    BCTrace trace;
    
    for (size_t i=0;i< raw.size();) {
        BCAddr curr = raw[i];
        if (glue.count(curr)) {
            BCBlock* block = db.getBlockById(blk_starts[curr]);
            trace.push_block(block->id);
            i += block->loc_count();
        } else {
            auto res = db.insert<BCBasicBlock>(blk_id, to_hex(curr));
            trace.push_block(res.id);
            if (res.created) blk_id++;
            i++;
        }
        sv.update_job_progress(i);
    }

    sv.update_job_progress(raw.size());
    
    return trace;
}

std::vector<uint32_t> flatten(const BCTrace& t) {
    std::vector<uint32_t> flat;

    for (auto& step : t.steps) {
        if (auto* p = std::get_if<uint32_t>(&step)) {
            flat.push_back(*p);
        } else {
            auto& li = std::get<BCLoopInstance>(step);
            flat.push_back(LOOP_ID_OFFSET + li.loop_id);
        }
    }
    return flat;  
}

BCTrace deloop(BCDatabase& db, const BCTrace& src, BCStatusViewModel& sv) {
    const int MAX_LOOP_SIZE = 25;
    const int MIN_LOOP_THRESHOLD = 10;
    BCTrace t = src;

    uint32_t loop_id = 1;

    for (int s=1;s<=MAX_LOOP_SIZE;s++) {

        sv.setup_job("Collapsing loops (window size " + std::to_string(s) + ")", t.size());

        std::vector<uint32_t> flat = flatten(t);

        BCTrace stage;
        const uint64_t n = flat.size();
        stage.steps.reserve(n);

        int total_its = 0;
        int i = 0;
        int loops_found = 0;
        int instances_found = 0;

        const uint32_t* data = flat.data();

        while (i < n) {
            if (i + 2*s <= n &&
                memcmp(data + i, data + i+s, s * sizeof(uint32_t)) == 0) {
                int is = i;
                int ic = 1;

                while (i + (ic+1)*s <= n &&
                    memcmp(data+is, data + i + ic*s, s * sizeof(uint32_t)) == 0) ic++;
                
                instances_found++;
                total_its += (ic - 1);

                if (ic >= MIN_LOOP_THRESHOLD) {
                    auto res = db.insert<BCLoop>(loop_id, "LOOP_" + std::to_string(loop_id), data+is, data+is+s);
                    stage.push_loop(res.id, ic);
                    if (res.created) {
                        loop_id++;
                        loops_found++;
                    }
                } else {
                    for (int iter=0;iter<ic;iter++) { 
                        for (int j=0;j<s;j++) stage.steps.push_back(data[is+j]);
                    }
                }
                i += ic * s;
            } else {
                stage.push_block(data[i]);
                i++;
            }
            sv.update_job_progress(i);
        }

        t = std::move(stage);

        // printf("Size %d: Found %d loops with %d usages and %d total iterations (vec size %zu)\n",
        //     s, loops_found, instances_found, total_its, t.steps.size());

    }

    BCTrace out;
    out.steps.reserve(t.size());

    sv.setup_job("De-flattening loops", t.size());
    int si = 0;
    int ti = 0;

    auto matches_body = [&](int start, const BCLoop* lp) -> bool {
        for (int j=0;j<lp->body.size();j++) {
            if (start+j >= src.steps.size()) return false;
            auto* val = std::get_if<uint32_t>(&src.steps[start+j]);
            if (!val) return false;
            if (*val != lp->body[j]) return false;
        }
        return true;
    };

    while (ti < t.size()) {
        std::visit(Overload {

            [&](uint32_t id) {
                if (id >= LOOP_ID_OFFSET) {

                    BCLoop* loop = db.getLoopById(id - LOOP_ID_OFFSET);
                    int loop_size = loop->body.size();
                    int it_count = 0;
                    while (si + loop_size <= (int)src.size() &&
                        matches_body(si, loop)) {
                        it_count++;
                        si += loop_size;
                    }

                    out.push_loop(loop->id, it_count);
                    ti++;

                } else {
                    out.push_block(id);
                    si++;
                    ti++;
                }
            },

            [&](BCLoopInstance li) {
                out.push_loop(li);
                si += db.getLoopById(li.loop_id)->body.size() * li.iterations;
                ti++;
            }
        }, t.steps[ti]);
        sv.update_job_progress(si);
    }

    return out;
}

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv) {

    std::vector<BCAddr> raw_data;
    std::ifstream f(path, std::ios::binary);

    f.seekg(0, std::ios::end);
    uint64_t size = f.tellg();
    f.seekg(4, std::ios::beg);

    uint8_t version; f.read(reinterpret_cast<char*>(&version), 1);
    uint8_t arch; f.read(reinterpret_cast<char*>(&arch), 1);
    bool is_x64 = (arch == 1);
    uint32_t hash; f.read(reinterpret_cast<char*>(&hash), 4);
    BCAddr base; f.read(reinterpret_cast<char*>(&base), 8);

    sv.setup_job("Loading trace", (size - (uint64_t)f.tellg()) / (is_x64 ? 8 : 4));
    int read = 0;

    BCAddr a = 0;
    while (f.read((char*)&a, (is_x64 ? 8 : 4))) {
        raw_data.push_back(a);
        sv.update_job_progress(read++);
        a = 0;
    }

    BCDatabase db;
    db.base_address = base;
    db.crc_hash = hash;

    printf("Architecture: %s \nHash: %04X \nBase: %08lX\n", (is_x64 ? "x64" : "x86"), hash, base);

    BCTrace t1 = build_chains(db, raw_data, sv);

    std::cout << "\nFound " << db.blocks.size() << " unique blocks and merged " << t1.steps.size()
        << " lines of trace\n";

    BCTrace t2 = deloop(db, t1, sv);

    std::cout
        << "\nAnalysis done with " << db.blocks.size() << " unique blocks and "
        << t2.steps.size() << " lines of trace. Saving..." << std::endl;

    db.apply_trace(t2);
    db.apply_prevs_nexts();

    db.find_hot_cold_blocks(sv);

    std::ofstream out("final_timeline.txt");
    
    for (const TraceStep& s : t2.steps) {
        std::visit(Overload {
            [&](uint32_t blk_id) { out << blk_id << '\n'; },
            [&](BCLoopInstance li) { out << 'L' << li.loop_id - LOOP_ID_OFFSET << " x" << li.iterations << '\n'; }
        }, s);
    }

    std::ofstream dict_out("structure_dictionary.txt");
    for (auto const& [id, block] : db.blocks) {
        dict_out << block->name << ": ";
        for (const auto& c : block->locs) dict_out << to_hex(c) << " ";
        dict_out << "\n";
    }

    for (auto const& [id, loop] : db.loops) {
        dict_out << loop->name << ": ";
        for (const auto& el : loop->body) dict_out << db.getBlockById(el)->name << " ";
        dict_out << "\n";
    }

    std::cout << "Output trace saved." << std::endl;
    return db;
}
