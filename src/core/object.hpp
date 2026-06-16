#pragma once

#include "address.hpp"
#include <cstdint>
#include <string>
#include <vector>

enum class BCObjectType : uint8_t { BasicBlock, Block, Loop, Module };

struct BCObjectId {
    uint64_t raw = 0;

    BCObjectId() = default;
    BCObjectId(uint32_t index, BCObjectType type)
        : raw((uint64_t)index | ((uint64_t)type << 32)) {}
    explicit BCObjectId(uint64_t raw) : raw(raw) {}
    
    uint32_t index() const { return (uint32_t)(raw & 0xFFFFFFFF); }
    BCObjectType type() const { return (BCObjectType)(raw >> 32); }

    bool operator==(const BCObjectId& o) const { return raw == o.raw; }
    bool operator!=(const BCObjectId& o) const { return raw != o.raw; }
    bool operator<(const BCObjectId& other) const { return raw < other.raw; }
    explicit operator uint64_t() const { return raw; }

};

namespace std {
    template<> struct hash<BCObjectId> {
        size_t operator()(const BCObjectId& id) const noexcept {
            return hash<uint64_t>{}( id.raw );
        }
    };
}
struct Neighbor {
    BCObjectId id;
    std::string name;
    int count;
};

template <typename T>
struct RankedValue {
    T value;
    double percentile; // -1.0 means not applied
};

class BCDetailsViewModel;
class BCDatabase;

struct BCObject {
public:

    BCObjectId id;
    std::string name;

    std::vector<Neighbor> prevs;
    std::vector<Neighbor> nexts;
    
    RankedValue<uint32_t> usage_count;

    BCObject(BCObjectId id, const std::string& name)
        : id(id), name(name) {}

    virtual std::vector<BCAddr> get_code_addrs() const = 0;
    virtual void dispatch_details(const BCDetailsViewModel& vm) const = 0;

    virtual ~BCObject() = default;
};


class BCBlock : public BCObject {
public:

    std::vector<BCObject*> members;

    RankedValue<uint32_t> instr_count;

    BCBlock(BCObjectId id, std::vector<BCObject*> members)
        : BCObject(id, "BLK_" + std::to_string(id.index())), members(std::move(members)) {}

    std::vector<BCAddr> get_code_addrs() const;
    void dispatch_details(const BCDetailsViewModel& vm) const;

};

class BCBasicBlock : public BCObject {
public:
    BCAddr address;
    BCBasicBlock(BCObjectId id, BCAddr address)
        : BCObject(id, to_hex(address)), address(address) {}

    std::vector<BCAddr> get_code_addrs() const;
    void dispatch_details(const BCDetailsViewModel& vm) const;

};
struct BCLoop : public BCObject {
public:
    std::vector<BCObject*> body;
    std::vector<BCObjectId> raw_body;

    template<typename Iterator>
    BCLoop(BCObjectId id, Iterator begin, Iterator end)
        : BCObject(id, "LOOP_" + std::to_string(id.index())), raw_body(begin, end) {}

    std::vector<BCAddr> get_code_addrs() const;
    void dispatch_details(const BCDetailsViewModel& vm) const;
    
};

struct BCLoopInstance {
    uint32_t loop_id;
    uint32_t iterations;
    BCObjectId full_id() const { return BCObjectId { loop_id, BCObjectType::Loop }; }
};

struct BCModule {
    BCAddr start;
    BCAddr end;
    std::string path;

    BCModule(BCAddr start, BCAddr end, const std::string& path)
        : start(start), end(end), path(path) {} 

};