#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "block.hpp"

class BCDatabase {
public:
    std::unordered_map<std::string, uint32_t> names;
    std::unordered_map<uint32_t, std::unique_ptr<BCBlock>> blocks;
    std::vector<uint32_t> trace;

    bool rename(uint32_t id, std::string newName) {
        auto it = blocks.find(id);
        if (it == blocks.end()) return false;
        std::unique_ptr<BCBlock>& block = it->second;
        
        names.erase(block->name);
        block->name = newName;
        names[newName] = id;
        return true;
    }

    BCBlock* getByName(const std::string& name) {
        // std::cout << "Nazwa: " << name << '\n';
        auto it = names.find(name);
        if (it == names.end()) return nullptr;

        return blocks[it->second].get();
    }

    BCBlock* getById(uint32_t id) {
        auto it = blocks.find(id);
        if (it == blocks.end()) return nullptr;

        return it->second.get();
    }

    template<typename T, typename... Args>
    void insert(uint32_t id, const std::string& name, Args&&... args) {

        if (blocks.find(id) != blocks.end()) {
            throw std::runtime_error("Block already exists");
            return;
        }

        if (names.find(name) != names.end()) {
            throw std::runtime_error("Name already exists");
            return;
        }

        auto block = std::make_unique<T>(id, name, std::forward<Args>(args)...);
        block->name = name;

        names[name] = id;
        blocks.emplace(id, std::move(block));
    }

    BCDatabase() = default;

};