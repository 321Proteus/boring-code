#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Neighbor {
    uint32_t id;
    std::string name;
    size_t count;
};

template <typename T>
struct RankedValue {
    T value;
    double percentile; // -1.0 means not applied
};

struct BCObject {
public:

    uint32_t id;
    std::string name;

    std::vector<Neighbor> prevs;
    std::vector<Neighbor> nexts;
    
    RankedValue<uint32_t> usage_count;

    BCObject(uint32_t id, const std::string& name)
        : id(id), name(name) {}

    virtual ~BCObject() = default;
};