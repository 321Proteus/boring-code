#pragma once

#include "core/database.hpp"
#include "core/loader.hpp"
#include "data/provider.hpp"
#include "core/view.hpp"
#include "providers/direct.hpp"
#include <map>
#include <memory>
#include <string>
#include <any>
#include <stdexcept>

class Session {
private:
    std::map<std::string, std::any> data;
public:

    std::unique_ptr<BCDatabase> database;
    BCTraceViewModel* trace_view = nullptr;
    BCDetailsViewModel* details_view = nullptr;
    BCStatusViewModel* status_view = nullptr;

    std::unique_ptr<BCCodeProviderRegistry> provider_registry;

    uint32_t checksum;

    template<typename T>
    void set(const std::string& key, T value) {
        data[key] = value;
    }
    
    template<typename T>
    T& get(const std::string& key) {
        auto it = data.find(key);
        if (it != data.end()) {
            return std::any_cast<T&>(it->second);
        }
        throw std::runtime_error("Key not found: " + key);
    }

    template<typename T>
    const T& get(const std::string& key) const {
        auto it = data.find(key);
        if (it != data.end()) {
            return std::any_cast<const T&>(it->second);
        }
        throw std::runtime_error("Key not found " + key);
    }
    
    bool has(const std::string& key) const {
        return data.find(key) != data.end();
    }

    void load_trace(const std::string& path, BCStatusViewModel* model = nullptr) {

        BCStatusViewModel& sv = model ? *model : *status_view;

        database = std::make_unique<BCDatabase>(load_database(path, sv));
        this->checksum = database->crc_hash;
        provider_registry = std::make_unique<BCCodeProviderRegistry>(resolve_modules(*database, sv));
        
    }

};
