#pragma once

#include "core/block.hpp"
#include "core/database.hpp"
#include <cstddef>
#include <iostream>
#include <any>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "core/loader.hpp"
#include "core/object.hpp"
#include "data/session.hpp"
#include "core/overload.hpp"

struct CommandArgs {
    std::string main_arg;
    std::vector<std::pair<std::string, std::string>> additional_args;
};

class Command {
public:

    Command() = default;

    virtual int exec(const CommandArgs& args, Session& sess) = 0;
    virtual std::any output() const = 0;

    const std::string helpText;

    virtual ~Command() = default;
};

class LoadTraceCommand : public Command {
public:

    int exec(const CommandArgs& args, Session& sess) override {

        std::string path = args.main_arg;

        if (path == "") {
            std::cerr << "No path has been provided!" << std::endl;
            return 0x1;
        }

        if (!std::filesystem::exists(path)) {
            std::cerr << "Could not find the requested file!" << std::endl;
            return 0x2;
        }

        if (detect_type(path) != BCFileType::BCTRACE) {
            std::cerr << "The provided file is not a valid BoringCode trace!" << std::endl;
            return 0x7;
        }

        auto sv = sess.status_view;

        sess.load_trace(path);
        sess.set("db_loaded", 1);
        return 0x0;
    }

    std::any output() const override {
        return NULL;
    }

    const std::string helpText = "Load a PIN-generated code execution trace from a given path";

};

class BlockDetailsCommand : public Command {
public:

    int exec(const CommandArgs& args, Session& sess) override {

        if (!sess.has("db_loaded")) {
            std::cerr << "No database loaded!" << std::endl;
            return 0x3;
        }

        BCDatabase* db = sess.database.get();
        std::string name = args.main_arg;
        auto it = db->names.find(name);
        if (it == db->names.end()) {
            std::cerr << "Object not found!" << std::endl;
            return 0x4;
        }
        BCObject* obj = db->resolve_object(it->second);
        sess.details_view->show_details(*obj);

        return 0x0;
    }

    std::any output() const override {
        return NULL;
    }

    const std::string helpText = "Display details about a block included in the loaded trace";

};

class ClearCommand : public Command {
public:

    int exec(const CommandArgs& args, Session& sess) override {
        std::cout << "\033[2J\033[1;1H";
        return 0x0;
    }

    std::any output() const override {
        return NULL;
    }

    const std::string helpText = "Clear the screen";

};

class TraceCommand : public Command {
public:

    int exec(const CommandArgs& args, Session& sess) override {
        BCDatabase* db = sess.database.get();
        if (!db) {
            std::cerr << "No database loaded!" << std::endl;
            return 0x3;
        }
        
        uint64_t start = 0;
        uint64_t n = 10;

        const bool TRACE_COLLAPSE_LOOP = false;

        if (start >= db->trace.size()) {
            std::cerr << "Invalid trace offset!" << std::endl;
            return 0x6;
        } else {
            for (int i=0;i<n;i++) {
                uint64_t offset = start + i;
                if (offset >= db->trace.size()) {
                    std::cout << "End of database\n";
                    break;
                } else {
                    TraceStep step = db->trace.steps[offset];
                    std::visit(Overload {
                        [&](uint32_t blk_id) {
                            std::cout << offset << '\t' << db->resolve_object(blk_id)->name << '\n';
                        },
                        [&](BCLoopInstance loop) {
                            if (TRACE_COLLAPSE_LOOP) {
                                std::cout << offset << '\t' << db->getLoopById(loop.loop_id)->name
                                    << " (x" << loop.iterations << ")\n"; 
                            } else {
                                for (int iter=0;iter<loop.iterations && i<n;iter++,i++) {
                                    std::cout << offset << '\t' << db->getLoopById(loop.loop_id)->name << '\n';
                                }
                            }
                        }
                    }, step);
                }
            }
        }

        return 0x0;

    }

    std::any output() const override {
        return NULL;
    }

    const std::string helpText = "Display a specific amount of blocks from the code execution trace at a given offset";
};

const std::map<std::string, std::string> short_names = {
    { "lt", "loadtrace" },
    { "lb", "loadbinary" },
    { "c", "clear" },
    { "b", "block" },
    { "s", "save" },
    { "tr", "trace" }
    // { "h", "help" }
};

const std::map<std::string, std::shared_ptr<Command>> commands = {
    { "loadtrace", std::make_shared<LoadTraceCommand>() },
    { "clear", std::make_shared<ClearCommand>() },
    { "block", std::make_shared<BlockDetailsCommand>() },
    { "trace", std::make_shared<TraceCommand>() }
    // { "help", std::make_shared<HelpCommand>() }
};

class HelpCommand : public Command {
public:

    int exec(const CommandArgs& args, Session& sess) override {

        if (args.main_arg != "") {

            bool found = false;
            for (const auto& [short_name, long_name] : short_names) {
                if (long_name == args.main_arg) {
                    std::cout << long_name << ", " << short_name << " - " << commands.at(long_name)->helpText << std::endl;
                }
            }

            if (!found) {
                std::cerr << "The target command is invalid!" << std::endl;
                return 0x5;
            }

        } else {
            for (const auto& [short_name, long_name] : short_names) {
                std::cout << long_name << ", " << short_name << " - " << commands.at(long_name)->helpText << std::endl;
            }
        }

        return 0x0;
    }

    std::any output() const override {
        return NULL;
    }

    const std::string helpText = "Prints help on a specific command or lists commands";

};
