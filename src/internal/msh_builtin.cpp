// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

/**
 * @brief Map of internal commands and their corresponding functions.
 */
const std::map<std::string, command::func_t> internal_commands = {
        {"merrno",   merrno},
        {"mpwd",     mpwd},
        {"mcd",      mcd},
        {"mexit",    mexit},
        {"mecho",    mecho},
        {"mexport",  mexport},
        {"msource",  msource},
        {".",        msource},
        {"malias",   malias},
        {"munalias", munalias},
};

/**
 * @brief Map of aliases.
 */
std::map<std::string, std::string> aliases;

/**
 * @brief Check if a command is a built-in command.
 * @param cmd Command to check.
 * @return True if command is built-in, false otherwise.
 */
bool is_builtin(const std::string &cmd) {
    return internal_commands.contains(cmd);
}

/**
 * @brief Check if help flag is present in arguments and print help message if it is.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @param doc Documentation of the command.
 * @return True if help flag is present, false otherwise.
 */
bool handle_help(int argc, char **argv, const builtin_doc &doc) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            std::cout << doc.args << " -- " << doc.brief << "\n\n";
            if (!doc.doc.empty()) {
                std::cout << doc.doc << "\n\n";
            }
            return true;
        }
    }
    return false;
}
