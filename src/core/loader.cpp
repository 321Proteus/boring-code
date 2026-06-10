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
#include "core/format.h"
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
std::vector<BCObjectId> flatten(const BCTrace& t, bool blocks_only = false) {
    std::vector<BCObjectId> flat;

    if (blocks_only) {
        for (auto& step : t.steps) {
            auto* p  = std::get_if<BCObjectId>(&step);
            flat.push_back(*p);
        }
    } else {
        for (auto& step : t.steps) {
            if (auto* p = std::get_if<BCObjectId>(&step)) [[likely]] {
                flat.push_back(*p);
            } else {
                auto& li = std::get<BCLoopInstance>(step);
                flat.push_back(li.loop_id);
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
            const BCObjectId* el = std::get_if<BCObjectId>(&src.steps[start+j]);
            if (!el) return false;
            if (*el != lp->raw_body[j]) return false;
        }
        return true;
    };

    sv.setup_job("Re-inflating loops", flat.size());
    int si = 0;
    int ti = 0;

    while (ti < flat.size()) {
        std::visit(Overload {

            [&](BCObjectId id) {
                if (id.type() == BCObjectType::Loop) {
                    BCLoop* loop = db.store.get_loop(id);
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
                    out.push_object(id);
                    si++;
                    ti++;
                }
            },

            [&](BCLoopInstance li) {
                out.push_loop(li);
                si += db.store.get_loop(li.loop_id)->raw_body.size() * li.iterations;
                ti++;
            }

        }, flat.steps[ti]);
        sv.update_job_progress(si);
    }

    return out;
}


void map_successors(BCDatabase& db, const std::vector<BCObjectId>& t, BCStatusViewModel& sv) {
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

    const std::vector<BCObjectId> flat = flatten(input, first_pass);

    map_successors(db, flat, sv);

    sv.setup_job("Scanning for constant successors", db.next_map.size());
    uint64_t cs_progress = 0;

    std::unordered_map<BCObjectId, BCObjectId> glue;
    for (auto const& [src, targets] : db.next_map) {
        if (targets.size() == 1) {
            BCObjectId dst = targets.begin()->first;
            if (src == dst) continue;
            if (db.prev_map[dst].size() == 1) glue[src] = dst;
        }
        sv.update_job_progress(cs_progress++);
    }
    sv.update_job_progress(db.next_map.size());

    std::unordered_map<BCObjectId, BCObjectId> blk_starts;
    uint32_t new_blocks = 0;

    std::unordered_set<BCObjectId> has_incoming_glue;
    for (auto const& [src, dst] : glue) {
        has_incoming_glue.insert(dst);
    }

    sv.setup_job("Generating block definitions", glue.size());
    uint64_t bd_progress = 0;

    for (auto const& [src, dst] : glue) {
        if (!has_incoming_glue.count(src)) {

            std::vector<BCObject*> members;
            BCObjectId curr = src;

            while (glue.count(curr)) {
                members.push_back(db.store.get(curr));
                curr = glue[curr];
            }

            members.push_back(db.store.get(curr));

            auto res = db.store.insert_block(members);
            blk_starts[src] = res.id;

            new_blocks++;

        }
        sv.update_job_progress(bd_progress++);
    }

    sv.update_job_progress(glue.size());
    sv.setup_job("Optimizing trace", flat.size());
    BCTrace trace;
    
    for (size_t i=0;i<flat.size();) {
        BCObjectId curr = flat[i];

        if (glue.count(curr)) {
            BCBlock* block = db.store.get_block(blk_starts[curr]);
            trace.push_object(block->id);
            i += block->members.size();
        } else {
            trace.push_object(curr);
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
    uint32_t loop_id = db.store.loops().size() + 1;
    for (int s=1;s<=MAX_LOOP_SIZE;s++) {
        sv.setup_job("Collapsing loops (window size " + std::to_string(s) + ")", t.size());
        std::vector<BCObjectId> flat = flatten(t);
        BCTrace stage;
        const uint64_t n = flat.size();
        stage.steps.reserve(n);
        int total_its = 0;
        int i = 0;
        int loops_found = 0;
        int instances_found = 0;
        const BCObjectId* data = flat.data();
        while (i < n) {
            if (i + 2*s <= n &&
                memcmp(data + i, data + i+s, s * sizeof(BCObjectId)) == 0) {
                int is = i;
                int ic = 1;
                while (i + (ic+1)*s <= n &&
                    memcmp(data+is, data + i + ic*s, s * sizeof(BCObjectId)) == 0) ic++;
                instances_found++;
                total_its += (ic - 1);
                if (ic >= MIN_LOOP_THRESHOLD) {
                    auto res = db.store.insert_loop(data+is, data+is+s);
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
                stage.push_object(data[i]);
                i++;
            }
            sv.update_job_progress(i);
        }
        int prev_idx = 0;
        for (auto& step : stage.steps) {
            if (auto* id = std::get_if<BCObjectId>(&step)) {
                if (id->type() == BCObjectType::Loop) {
                    while (id->index() < loop_id && prev_idx < t.steps.size()) {
                        if (auto* li = std::get_if<BCLoopInstance>(&t.steps[prev_idx])) {
                            if (li->loop_id == *id) {
                                step = BCLoopInstance{ *id, li->iterations };
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
    printf("Found %zu loops and compressed down to %zu lines of trace\n", db.store.loops().size(), t.size());
    return t;
}

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv) {

    BCTrace raw_data;
    std::ifstream f(path, std::ios::binary);

    f.seekg(0, std::ios::end);
    uint64_t fsize = f.tellg();
    f.seekg(0, std::ios::beg);

    Header hdr; f.read(reinterpret_cast<char*>(&hdr), sizeof(Header));

    BCDatabase db;
    db.base_address = hdr.base_low | ((uint64_t)hdr.base_mid << 32);
    db.crc_hash = hdr.hash;

    uint64_t total_size = fsize - (uint64_t)f.tellg();
    std::vector<char> buffer(total_size);
    f.read(buffer.data(), buffer.size());

    const char* ptr = buffer.data();
    const char* end = ptr + buffer.size();

    uint64_t trace_length = buffer.size() / sizeof(bc_block_trace_t); // Warning: rough
    raw_data.steps.reserve(trace_length);
    sv.setup_job("Loading trace", trace_length);

    int prog = 0;

    while (ptr < end) {

        bc_trace_event_header_t eh = *reinterpret_cast<const bc_trace_event_header_t*>(ptr);
        ptr += sizeof(bc_trace_event_header_t);

        switch (eh.type) {
            case EV_BBL: {
                printf("[BBL] x%d\n\n", eh.count);
                while (eh.count--) {
                    const bc_block_trace_t* bb = reinterpret_cast<const bc_block_trace_t*>(ptr);
                    BCAddr addr = (BCAddr)bb->pc_low | ((BCAddr)bb->pc_mid << 32);
                    BCInsertionResult r = db.store.insert_basic_block(addr);
                    raw_data.push_object(r.id);
                    ptr += sizeof(bc_block_trace_t);

                    sv.update_job_progress(prog++);
                }
                break;
            }
            case EV_MODULE: {
                const bc_module_trace_t* mod = reinterpret_cast<const bc_module_trace_t*>(ptr);
                
                printf("[MODULE #%d]\nStart: %lX\nEnd: %lX\n", mod->module_id, mod->start, mod->end);
                std::string path(mod->path, mod->path_size);
                printf("Path: %s\n\n", path.c_str());

                int total_size = sizeof(bc_module_trace_t) + mod->path_size;
                ptr += total_size;
                break;

                prog += (total_size / sizeof(bc_block_trace_t));

                sv.update_job_progress(prog);
            }
        }

    }

    printf("Architecture: %s \nHash: %X \nBase: %zX\n", (hdr.arch ? "x64" : "x86"), db.crc_hash, db.base_address);
    printf("Loaded %zu basic blocks and %zu trace steps\n", db.store.basic_blocks().size(), raw_data.size());

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
            [&](BCObjectId id) {
                out << db.store.get(id)->name << '\n';
            },
            [&](BCLoopInstance li) {
                out << db.store.get_loop(li.loop_id)->name << " x" << li.iterations << '\n';
            }
        }, s);
    }

    std::cout << "Output trace saved\n";

    std::ofstream dict_out("structure_dictionary.txt");
    for (auto const& block : db.store.blocks()) {
        dict_out << block.get()->name << ": ";
        for (const BCObject* m : block.get()->members) {
            dict_out << m->name << ' ';
        }
        dict_out << '\n';
    }
    
    for (auto const& loop : db.store.loops()) {
        dict_out << loop.get()->name << ": ";
        for (const BCObject* m : loop.get()->body) {
            dict_out << m->name << ' ';
        }
        dict_out << '\n';
    }

    std::cout << "Dictionary saved" << std::endl;
    return db;
}
