//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

std::map<std::string, int (*)(int, char **)> internal_commands = {
        {"merrno",  merrno},
        {"mpwd",    mpwd},
        {"mcd",     mcd},
        {"mexit",   mexit},
        {"mecho",   mecho},
        {"mexport", mexport},
        {"msource", msource},
        {".",       msource},
};

bool is_internal(const std::string &cmd) {
    return internal_commands.find(cmd) != internal_commands.end();
}

bool handle_help(int argc, char **argv, const char *doc, const char *args_doc) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            std::cout << doc << std::endl;
            std::cout << "Usage: " << argv[0] << " " << args_doc << std::endl;
            return true;
        }
    }
    return false;
}
