// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>

static builtin_doc doc = {
        .args   = "mexit [code] [-h|--help]",
        .brief  = "Exit the shell",
        .doc    = "Exits the shell with a status of code given as an argument.\n"
                  "If no argument is given exits with a status of 0.\n"
                  "Doesn't return unless given wrong number of arguments or code is invalid."
};

int mexit(int argc, char **argv) {
    if (handle_help(argc, argv, doc)) {
        return 0;
    }

    if (argc == 1) {
        exit(0);
    }
    if (argc > 2) {
        std::cout << "Usage: mexit <code>" << std::endl;
        return 1;
    }

    try {
        exit(std::stoi(argv[1]));
    } catch (std::invalid_argument &e) {
        std::cout << "mexit: invalid argument: " << argv[1] << std::endl;
        return 1;
    } catch (std::out_of_range &e) {
        std::cout << "mexit: argument out of range: " << argv[1] << std::endl;
        return 1;
    }
}
