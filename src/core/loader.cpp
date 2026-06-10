#include "loader.hpp"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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
#include "object.hpp"
#include "view.hpp"
#include "overload.hpp"

#include "database.hpp"
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
                flat.push_back(li.loop_id + LOOP_ID_OFFSET);
            }
        }
    }

    return flat;  
}

BCTrace inflate(const BCDatabase& db, const BCTrace& flat, const BCTrace& src, BCStatusViewModel& sv) {
    
    BCTrace out;
    out.steps.reserve(src.size());
    
    auto matches_body = [&](int start, const BCLoop* lp) -> bool {
        for (int j=0;j<lp->raw_body.size();j++) {
            if (start+j >= src.steps.size()) return false;
            auto* val = std::get_if<uint32_t>(&src.steps[start+j]);
            if (!val) return false;
            if (*val != lp->raw_body[j]) return false;
        }
        return true;
    };

    sv.setup_job("Re-inflating loops", flat.size());
    int si = 0;
    int ti = 0;

    while (ti < flat.size()) {
        std::visit(Overload {

            [&](uint32_t id) {
                if (id >= LOOP_ID_OFFSET) {
                    BCLoop* loop = db.getLoopById(id - LOOP_ID_OFFSET);
                    int loop_size = loop->raw_body.size();
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
                si += db.getLoopById(li.loop_id)->raw_body.size() * li.iterations;
                ti++;
            }

        }, flat.steps[ti]);
        sv.update_job_progress(si);
    }

    return out;
}


void map_successors(BCDatabase& db, const std::vector<uint32_t>& t, BCStatusViewModel& sv) {
    sv.setup_job("Mapping successors", t.size());

    db.next_map.clear();
    db.prev_map.clear();

    for (size_t i=0;i<t.size()-1;i++) {
        db.next_map[t[i]][t[i+1]]++;
        db.prev_map[t[i+1]][t[i]]++;
        sv.update_job_progress(i);
    }

    sv.update_job_progress(t.size());
}

BCTrace build_chains(BCDatabase& db, const BCTrace& input, BCStatusViewModel& sv, bool first_pass) {

    const std::vector<uint32_t> flat = flatten(input, first_pass);

    map_successors(db, flat, sv);

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

    printf("Found %d new blocks and merged %zu lines of trace\n", new_blocks, trace.size());

    sv.update_job_progress(flat.size());
    
    return inflate(db, trace, input, sv);

}

BCTrace deloop(BCDatabase& db, const BCTrace& src, BCStatusViewModel& sv) {
    
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


    printf("Found %zu loops and compressed down to %zu lines of trace\n", db.loops.size(), t.size());

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
    db.is_x64 = is_x64;
    db.base_address = base;
    db.crc_hash = hash;

    sv.setup_job("Loading trace", (size - (uint64_t)f.tellg()) / (is_x64 ? 8 : 4));

    std::vector<char> buffer(size - f.tellg());
    f.read(buffer.data(), buffer.size());

    const char* ptr = buffer.data();
    const char* end = ptr + buffer.size();

    raw_data.steps.reserve(buffer.size() / (is_x64 ? 8 : 4));

    int count = 0;
    BCAddr a = 0;
    uint32_t bblk_id = 1;
    while (ptr < end) {

        BCAddr a = is_x64 ? *reinterpret_cast<const uint64_t*>(ptr) : *reinterpret_cast<const uint32_t*>(ptr);
        ptr += (is_x64 ? 8 : 4);
        auto res = db.insert<BCBasicBlock>(bblk_id, a);
        raw_data.push_block(res.id);
        if (res.created) bblk_id++;
        sv.update_job_progress(count++);
    }

    printf("Architecture: %s \nHash: %X \nBase: %016llX\n", (is_x64 ? "x64" : "x86"), hash, base);
    printf("Loaded %zu basic blocks an %zu trace steps\n", db.basic_blocks.size(), raw_data.size());

    BCTrace t1 = build_chains(db, raw_data, sv, true);
    BCTrace t3 = deloop(db, t1, sv);
    // BCTrace t3 = build_chains(db, t2, sv, false);

    std::cout << "Saving...\n";

    db.apply_trace(t3);
    db.apply_prevs_nexts(sv);
    db.find_hot_cold_blocks(sv);

    std::ofstream out("final_timeline.txt");
    
    for (const TraceStep& s : t3.steps) {
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
