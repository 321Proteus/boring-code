#pragma once

#include <cstdint>
#include <string>

struct BCObject {
public:
    uint32_t id;
    std::string name;

    BCObject(uint32_t id, const std::string& name)
        : id(id), name(name) {}

    virtual ~BCObject() = default;
};