#pragma once

#include "core/block.hpp"
#include "ui/view.hpp"
#include "overload.hpp"
#include <variant>

enum class BCFileType {
    ELF, PE, BCTRACE, UNKNOWN
};

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

    void push_loop(uint32_t loop_id, uint32_t iterations) {
        steps.emplace_back(BCLoopInstance { loop_id, iterations });
    }

};

#include "database.hpp"

BCFileType detect_type(const std::string& path);

BCDatabase load_database(const std::string& path, BCStatusViewModel& sv);