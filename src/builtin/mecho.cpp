// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>

static const builtin_doc doc = {
        .args   = "mecho [args] [-h|--help]",
        .brief  = "Write arguments to the standard output",
        .doc    = "Arguments are separated by a single space character.\n"
                  "If no arguments are given, a blank line is output.\n"
};

int mecho(int argc, char **argv) {
    if (handle_help(argc, argv, doc)) {
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}
