// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//
/**
 * @file
 * @brief Built-in commands and related utilities.
 */

#include "internal/msh_builtin.h"

#include <boost/program_options.hpp>

/**
 * @brief Internal map of built-in commands.
 *
 * Maps command names to their corresponding functions.
 */
const std::map<std::string, func_t> builtin_commands = {
        {"merrno",   &merrno},
        {"mpwd",     &mpwd},
        {"mcd",      &mcd},
        {"mexit",    &mexit},
        {"mecho",    &mecho},
        {"mexport",  &mexport},
        {"msource",  &msource},
        {".",        &msource},
        {"malias",   &malias},
        {"munalias", &munalias},
        {"mjobs",    &mjobs},
};

/**
 * @brief Internal map of aliases.
 *
 * Maps alias names to their corresponding commands.
 */
std::map<std::string, std::string> aliases;

/**
 * @brief Check if a command is a built-in command.
 * @param cmd Command to check.
 * @return True if command is built-in, false otherwise.
 */
bool is_builtin(const std::string &cmd) {
    return builtin_commands.contains(cmd);
}

/**
 * @brief Check if help flag is present in arguments and print help message if it is.
 *
 * If unknown flags are present, throws an exception intended to be caught by the
 * internal command handler.
 *
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @param doc Documentation of the command.
 * @return True if help flag is present, false otherwise.
 *
 * @throws boost::program_options::error if arguments are invalid.
 */
bool handle_help(int argc, char **argv, const builtin_doc &doc) {
    using namespace boost;
    namespace po = program_options;

    po::options_description desc("Options");
    desc.add_options()
            ("help,h", "Print help message");

    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        if (vm.count("help")) {
            std::cout << doc.name << " " << doc.args << " -- " << doc.brief << "\n\n";
            if (!doc.doc.empty()) {
                std::cout << doc.doc << "\n\n";
            }
            return true;
        } else {
            return false;
        }
    } catch (const po::error &) {
        throw;
    }
}
