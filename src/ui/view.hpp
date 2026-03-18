#pragma once

#include "core/block.hpp"

class BCTraceView {
public:
    virtual void show_trace() const = 0;
};

class BCDetailsView {
public:
    virtual void show_details(const BCBlock::Details& details) const = 0;
    virtual void show_details(const BCBasicBlock& block) const = 0;
};

class BCStatusView {
private:
    std::pair<uint64_t, uint64_t> job_progress;
public:
    virtual void setup_job(uint64_t size) = 0;
    virtual void update_job_progress(uint64_t new_progress) = 0;
    virtual void show_error(const std::string& msg) const = 0;
};