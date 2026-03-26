#pragma once

#include "core/block.hpp"
#include "core/database.hpp"
#include "core/loader.hpp"
#include <cstddef>
#include <iostream>
#include <any>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "data/session.hpp"

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

class LoadDatabaseCommand : public Command {
public:

    int exec(const CommandArgs& args, Session& sess) override {

        if (args.main_arg == "") {
            std::cerr << "No path has been provided!" << std::endl;
            return 0x1;
        }

        if (!std::filesystem::exists(args.main_arg)) {
            std::cerr << "Could not find the requested file!" << std::endl;
            return 0x2;
        }

        auto sv = sess.status_view;

        sess.load_trace(args.main_arg);
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
        BCBlock* block = db->getByName(args.main_arg);

        if (!block) {
            std::cerr << "Block not found!" << std::endl;
            return 0x4;
        }
        BCBlock::Details d = db->generate_details(*block);
        sess.details_view->show_details(d);

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

const std::map<std::string, std::string> short_names = {
    { "lt", "loadtrace" },
    { "lb", "loadbinary" },
    { "c", "clear" },
    { "b", "block" },
    { "s", "save" },
    // { "h", "help" }
};

const std::map<std::string, std::shared_ptr<Command>> commands = {
    { "loadtrace", std::make_shared<LoadDatabaseCommand>() },
    { "clear", std::make_shared<ClearCommand>() },
    { "block", std::make_shared<BlockDetailsCommand>() },
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
