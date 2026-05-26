#pragma once

#include "core/view.hpp"
#include "overload.hpp"
#include <variant>
#include <vector>

enum class BCFileType {
    ELF, PE, BCTRACE, UNKNOWN
};

const uint32_t BLOCK_ID_OFFSET  = 0x40000000;
const uint32_t LOOP_ID_OFFSET   = 0x80000000;

const int MAX_LOOP_SIZE         = 25;
const int MIN_LOOP_THRESHOLD    = 15;

using TraceStep = std::variant<uint32_t, BCLoopInstance>;

inline uint32_t get_id(const TraceStep& step) {
    uint32_t id = 0;
    std::visit(Overload {
        [&](uint32_t blk_id) { id = blk_id; },
        [&](const BCLoopInstance& loop) { id = loop.loop_id; }
    }, step);
    return id;
}

class BCTrace {
public:
    std::vector<TraceStep> steps;

    size_t size() const { return steps.size(); }

    void push_block(uint32_t id) {
        steps.push_back(id);
    }

    void push_loop(BCLoopInstance instance) {
        steps.emplace_back(instance);
    }
    
    void push_loop(uint32_t loop_id, uint32_t iterations) {
        steps.emplace_back(BCLoopInstance { loop_id, iterations });
    }

};

#include "database.hpp"

BCFileType detect_type(const std::string& path);

std::vector<uint32_t> flatten(const BCTrace& t, bool blocks_only);

void map_successors(BCDatabase& db, const std::vector<uint32_t>& t, BCStatusViewModel& sv);

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv);