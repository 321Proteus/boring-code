#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "commands.hpp"
#include "data/session.hpp"

int main(int argc, char* argv[]) {

    Session sess;

    while (true) {

        std::cout << "boring-code > ";
        std::string cmd; std::cin >> cmd;

        std::string raw_args; std::getline(std::cin, raw_args);
        std::istringstream iss(raw_args);

        std::vector<std::string> split;
        std::string temp;

        while (iss >> temp) split.push_back(temp);
            
        //     /*  TODO: Implement logic for both implicit and explicit optional arguments like: "load trace.bin -p".
        //         None of the commands implement any so far, but their support might be needed in the future
        //     */

        CommandArgs args = { (split.empty() ? "" : split.front()), {} };

        if (commands.find(cmd) != commands.end()) {
            commands.at(cmd)->exec(args, sess);
        } else if (short_names.find(cmd) != short_names.end()) {
            commands.at(short_names.at(cmd))->exec(args, sess);
        } else {
            std::cerr << "Invalid command!" << std::endl;
        }

    }
}
