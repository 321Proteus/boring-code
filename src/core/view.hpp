#pragma once

#include "object.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

typedef struct {
    uint32_t id;
    std::string name;
} BCTraceEntry;

typedef struct {
    std::string name;
    uint64_t size;
    uint64_t progress;
} BCJob;

class BCDetailsViewModel {
public:
    virtual void show_details(const BCBlock& block) const = 0;
    virtual void show_details(const BCBasicBlock& bb) const = 0;
    virtual void show_details(const BCLoop& loop) const = 0;
};

class BCStatusViewModel {
private:
    BCJob job;
public:
    virtual void setup_job(const std::string name, uint64_t size) = 0;
    virtual void update_job_progress(uint64_t progress) = 0;
    virtual void show_error(const std::string& msg) = 0;
};

class BCDatabase;  // Forward declaration

// TODO: work on this
class BCTraceViewModel {
public:
    static std::shared_ptr<std::vector<BCTraceEntry>> precompute_trace(const BCDatabase& db, BCStatusViewModel& sv);
    virtual void show_trace(std::shared_ptr<std::vector<BCTraceEntry>> trace) const = 0;
};