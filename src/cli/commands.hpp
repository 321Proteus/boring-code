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

using std::string;
using std::vector;

struct CommandArgs {
    string main_arg;
    vector<std::pair<string, string>> additional_args;
};

class Command {
public:

    Command() = default;

    virtual int exec(const CommandArgs& args, Session& sess) = 0;
    virtual std::any output() const = 0;

    string helpText;

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

        sess.database = std::make_unique<BCDatabase>(load_database(args.main_arg));
        sess.set("db_loaded", 1);
        return 0x0;
    }

    std::any output() const override {
        return NULL;
    }

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

        std::cout << block->info(db);
        return 0x0;
    }

    std::any output() const override {
        return NULL;
    }

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

};

const std::map<string, string> short_names = {
    { "lt", "loadtrace" },
    { "lb", "loadbinary" },
    { "c", "clear" },
    { "b", "block" },
    { "s", "save" }
};

const std::map<std::string, std::shared_ptr<Command>> commands = {
    { "loadtrace", std::make_shared<LoadDatabaseCommand>() },
    { "clear", std::make_shared<ClearCommand>() },
    { "block", std::make_shared<BlockDetailsCommand>() }
};

// void clear_terminal();

// void get_block(const BCDatabase& db, string block_name);
