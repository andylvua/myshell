// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//
/**
 * @file
 * @brief Built-in command `mexit`.
 * @ingroup builtin
 */

#include "internal/msh_builtin.h"

#include <iostream>

static const builtin_doc doc = {
        .name   = "mexit",
        .args   = "[code] [-h|--help]",
        .brief  = "Exit the shell",
        .doc    = "Exits the shell with a status of code given as an argument.\n"
                  "If no argument is given exits with a status of 0.\n"
                  "Doesn't return unless given wrong number of arguments or code is invalid."
};

int mexit(int argc, char **argv) {
    try {
        if (handle_help(argc, argv, doc)) {
            return 0;
        }
    } catch (const std::exception &e) {
        msh_error(doc.name + ": " + e.what());
        std::cerr << "Usage: " << doc.name << " " << doc.args << std::endl;
        return 1;
    }

    if (argc == 1) {
        exit(0);
    }
    if (argc > 2) {
        msh_error(doc.name + ": wrong number of arguments");
        std::cerr << doc.get_usage() << std::endl;
        return 1;
    }

    try {
        exit(std::stoi(argv[1]));
    } catch (const std::invalid_argument &) {
        msh_error(doc.name + ": invalid argument: " + argv[1]);
        exit(2);
    } catch (const std::out_of_range &) {
        msh_error(doc.name + ": argument out of range: " + argv[1]);
        exit(2);
    }
}
