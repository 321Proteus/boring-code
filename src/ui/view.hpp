#pragma once

#include "core/block.hpp"

typedef struct {
    std::string name;
    uint64_t size;
    uint64_t progress;
} BCJob;

class BCTraceViewModel {
public:
    virtual void show_trace() const = 0;
};

class BCDetailsViewModel {
public:
    virtual void show_details(const BCBlock::Details& details) = 0;
    virtual void show_details(const BCBasicBlock& block) = 0;
};

class BCStatusViewModel {
private:
    BCJob job;
public:
    virtual void setup_job(const std::string name, uint64_t size) = 0;
    virtual void update_job_progress(uint64_t progress) = 0;
    virtual void show_error(const std::string& msg) = 0;
};