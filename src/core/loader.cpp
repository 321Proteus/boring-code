#include "loader.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ios>
#include <ostream>
#include <unordered_map>
#include <cstddef>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>
#include "core/object.hpp"
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

std::vector<uint32_t> flatten(const BCTrace& t, bool blocks_only = false) {
    std::vector<uint32_t> flat;

    if (blocks_only) {
        for (auto& step : t.steps) {
            auto* p  = std::get_if<uint32_t>(&step);
            assert(p != nullptr);
            flat.push_back(*p);
        }
    } else {
        for (auto& step : t.steps) {
            if (auto* p = std::get_if<uint32_t>(&step)) [[likely]] {
                flat.push_back(*p);
            } else {
                auto& li = std::get<BCLoopInstance>(step);
                flat.push_back(LOOP_ID_OFFSET + li.loop_id);
            }
        }
    }

    return flat;  
}

BCTrace build_chains(BCDatabase& db, const BCTrace& input, BCStatusViewModel& sv, bool first_pass) {

    const std::vector<uint32_t> flat = flatten(input, first_pass);
    sv.setup_job("Mapping successors", flat.size());

    for (size_t i=0;i<flat.size()-1;i++) {
        db.next_map[flat[i]][flat[i+1]]++;
        db.prev_map[flat[i+1]][flat[i]]++;
        sv.update_job_progress(i);
    }

    sv.update_job_progress(flat.size());

    sv.setup_job("Scanning for constant successors", db.next_map.size());
    uint64_t cs_progress = 0;

    std::unordered_map<uint32_t, uint32_t> glue;
    for (auto const& [src, targets] : db.next_map) {
        if (targets.size() == 1) {
            uint32_t dst = targets.begin()->first;
            if (src == dst) continue;
            if (db.prev_map[dst].size() == 1) glue[src] = dst;
        }
        sv.update_job_progress(cs_progress++);
    }
    sv.update_job_progress(db.next_map.size());

    std::unordered_map<uint32_t, uint32_t> blk_starts;
    uint32_t blk_id = db.blocks.size() + 1;
    uint32_t new_blocks = 0;

    std::unordered_set<uint32_t> has_incoming_glue;
    for (auto const& [src, dst] : glue) {
        has_incoming_glue.insert(dst);
    }

    sv.setup_job("Generating block definitions", glue.size());

    for (auto const& [src, dst] : glue) {
        if (!has_incoming_glue.count(src)) {

            std::vector<BCObject*> members;
            BCAddr curr = src;

            while (glue.count(curr)) {
                members.push_back(db.resolve_object(curr));
                curr = glue[curr];
            }
            members.push_back(db.resolve_object(curr));
            blk_starts[src] = blk_id;

            db.insert<BCBlock>(blk_id, members);
            new_blocks++;
            blk_id++;

        }
        sv.update_job_progress(blk_id);
    }

    sv.update_job_progress(glue.size());
    sv.setup_job("Optimizing trace", flat.size());
    BCTrace trace;
    
    for (size_t i=0;i<flat.size();) {
        BCAddr curr = flat[i];

        if (glue.count(curr)) {
            BCBlock* block = db.getBlockById(blk_starts[curr]);
            trace.push_block(block->id + BLOCK_ID_OFFSET);
            i += block->members.size();
        } else {
            trace.push_block(curr);
            i++;
        }
        sv.update_job_progress(i);
    }

    std::cout << "Added " << new_blocks << " new blocks\n";

    sv.update_job_progress(flat.size());
    
    return trace;
}

BCTrace deloop(BCDatabase& db, const BCTrace& src, BCStatusViewModel& sv) {

    const int MAX_LOOP_SIZE = 25;
    const int MIN_LOOP_THRESHOLD = 3;
    BCTrace t = src;

    uint32_t loop_id = db.loops.size() + 1;

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
                    auto res = db.insert<BCLoop>(loop_id, data+is, data+is+s);
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

        int prev_idx = 0;
        for (auto& step : stage.steps) {
            if (auto* id = std::get_if<uint32_t>(&step)) {
                if (*id >= LOOP_ID_OFFSET) {
                    uint32_t lid = *id - LOOP_ID_OFFSET;
                    while (lid < loop_id && prev_idx < t.steps.size()) {
                        if (auto* li = std::get_if<BCLoopInstance>(&t.steps[prev_idx])) {
                            if (li->loop_id == lid) {
                                step = BCLoopInstance{ lid, li->iterations };
                                prev_idx++;
                                break;
                            }
                        }
                        prev_idx++;
                    }
                }
            }
        }

        t = std::move(stage);

        // printf("Size %d: Found %d loops with %d usages and %d total iterations (vec size %zu)\n",
        //     s, loops_found, instances_found, total_its, t.steps.size());

    }
    return t;

}

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv) {

    BCTrace raw_data;
    std::ifstream f(path, std::ios::binary);

    f.seekg(0, std::ios::end);
    uint64_t size = f.tellg();
    f.seekg(4, std::ios::beg);

    uint8_t version; f.read(reinterpret_cast<char*>(&version), 1);
    uint8_t arch; f.read(reinterpret_cast<char*>(&arch), 1);
    bool is_x64 = (arch == 1);
    uint32_t hash; f.read(reinterpret_cast<char*>(&hash), 4);
    BCAddr base; f.read(reinterpret_cast<char*>(&base), 8);

    BCDatabase db;
    db.base_address = base;
    db.crc_hash = hash;

    sv.setup_job("Loading trace", (size - (uint64_t)f.tellg()) / (is_x64 ? 8 : 4));
    int read = 0;

    BCAddr a = 0;
    uint32_t bblk_id = 1;

    while (f.read((char*)&a, (is_x64 ? 8 : 4))) {
        auto res = db.insert<BCBasicBlock>(bblk_id, a);
        raw_data.push_block(res.id);
        if (res.created) bblk_id++;
        sv.update_job_progress(read++);
        a = 0;
    }

    std::cout << "Loaded " << bblk_id-1 << " basic blocks\n";

    printf("Architecture: %s \nHash: %04X \nBase: %08lX\n", (is_x64 ? "x64" : "x86"), hash, base);

    BCTrace t1 = build_chains(db, raw_data, sv, true);

    std::cout << "\nFound " << db.blocks.size() << " unique blocks and merged " << t1.steps.size()
        << " lines of trace\n";

    BCTrace t2 = deloop(db, t1, sv);

    db.apply_prevs_nexts();

    // t2 = build_chains(db, t2, sv, false);

    std::cout
        << "\nAnalysis done with " << db.blocks.size() << " unique blocks and "
        << t2.steps.size() << " lines of trace. Saving..." << std::endl;

    db.apply_trace(t2);
    db.apply_prevs_nexts();

    // db.find_hot_cold_blocks(sv);

    std::ofstream out("final_timeline.txt");
    
    for (const TraceStep& s : t2.steps) {
        std::visit(Overload {
            [&](uint32_t blk_id) {
                if (blk_id >= BLOCK_ID_OFFSET) {
                    out << db.getBlockById(blk_id - BLOCK_ID_OFFSET)->name << '\n';
                } else {
                    out << db.getBasicBlockById(blk_id)->name << '\n';
                }
            },
            [&](BCLoopInstance li) {
                out << db.getLoopById(li.loop_id)->name << " x" << li.iterations << '\n';
            }
        }, s);
    }

    std::cout << "Output trace saved\n";

    std::ofstream dict_out("structure_dictionary.txt");
    for (auto const& [id, block] : db.blocks) {
        dict_out << block.get()->name << ": ";
        for (const BCObject* m : block.get()->members) {
            dict_out << m->name << ' ';
        }
        dict_out << '\n';
    }
    
    for (auto const& [id, loop] : db.loops) {
        dict_out << loop.get()->name << ": ";
        for (const BCObject* m : loop.get()->body) {
            dict_out << m->name << ' ';
        }
        dict_out << '\n';
    }

    std::cout << "Dictionary saved" << std::endl;
    return db;
}
