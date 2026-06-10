#pragma once

#include "core/object.hpp"
#include "view.hpp"
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

using TraceStep = std::variant<BCObjectId, BCLoopInstance>;

inline BCObjectId get_id(const TraceStep& step) {
    return std::visit(Overload {
        [&](BCObjectId blk_id) { return blk_id; },
        [&](const BCLoopInstance& li) { return li.loop_id; }
    }, step);
}

class BCTrace {
public:
    std::vector<TraceStep> steps;

    size_t size() const { return steps.size(); }

    void push_object(BCObjectId id) {
        steps.push_back(id);
    }

    void push_loop(BCLoopInstance instance) {
        steps.emplace_back(instance);
    }
    
    void push_loop(BCObjectId loop_id, uint32_t iterations) {
        steps.emplace_back(BCLoopInstance { loop_id, iterations });
    }

};

#include "database.hpp"

BCFileType detect_type(const std::string& path);

std::vector<BCObjectId> flatten(const BCTrace& t, bool blocks_only);

void map_successors(BCDatabase& db, const std::vector<BCObjectId>& t, BCStatusViewModel& sv);

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv);