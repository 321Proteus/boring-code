#pragma once

#include "core/block.hpp"
#include "ui/view.hpp"

class ConsoleViewModel :
    public BCTraceViewModel,
    public BCDetailsViewModel,
    public BCStatusViewModel
{
private:
    BCJob job;
public:
    void show_trace(std::shared_ptr<std::vector<BCTraceEntry>> trace) const;
    void show_details(const BCBlock::Details& details);
    void show_details(const BCBasicBlock& details);
    void setup_job(const std::string name, uint64_t size);
    void update_job_progress(uint64_t progress);
    void show_error(const std::string& msg);
};
