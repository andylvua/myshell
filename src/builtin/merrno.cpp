// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>

int msh_errno = 0;

static builtin_doc doc = {
        .args   = "merrno [-h|--help]",
        .brief  = "Print error code of the last command",
        .doc    = "Returns 1 if any arguments specified, 0 otherwise."
};

int merrno(int argc, char **argv) {
    if (handle_help(argc, argv, doc)) {
        return 0;
    }

    if (argc > 1) {
        std::cout << "Usage: merrno" << std::endl;
        return 1;
    }
    std::cout << msh_errno << std::endl;
    return 0;
}
