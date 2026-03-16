#include <iostream>
#include "../core/block.hpp"
#include "../core/loader.hpp"

int main(int argc, char* argv[]) {
    BCDatabase main = load_database("functions.bin");
    while (true) {
        std::cout << "Enter a block name to search: ";
        std::string block_name; std::cin >> block_name;
        BCBlock* block = main.getByName(block_name);

        if (!block) {
            std::cout << "Block not found!\n";
            continue;
        }

        std::cout << block->info(main);

    }
}
