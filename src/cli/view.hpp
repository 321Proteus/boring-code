#pragma once

#include "core/block.hpp"
#include "ui/view.hpp"

class BCConsoleView :
    public BCTraceView,
    public BCDetailsView,
    public BCStatusView
{
private:
    std::pair<uint64_t, uint64_t> job_progress;
public:
    void show_trace() const;
    void show_details(const BCBlock::Details& details) const;
    void show_details(const BCBasicBlock& details) const;
    void setup_job(uint64_t size);
    void update_job_progress(uint64_t new_progress);
    void show_error(const std::string& msg) const;
};