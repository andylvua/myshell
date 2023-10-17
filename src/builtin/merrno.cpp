// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>

int msh_errno = 0;

static const builtin_doc doc = {
        .name   = "merrno",
        .args   = "[-h|--help]",
        .brief  = "Print error code of the last command",
        .doc    = "Returns 1 if any arguments specified, 0 otherwise."
};

int merrno(int argc, char **argv) {
    try {
        if (handle_help(argc, argv, doc)) {
            return 0;
        }
    } catch (std::exception &e) {
        print_error(doc.name + ": " + e.what());
        std::cerr << "Usage: " << doc.name << " " << doc.args << std::endl;
        return 1;
    }

    if (argc > 1) {
        std::cerr << "merrno: wrong number of arguments" << std::endl;
        std::cerr << doc.get_usage() << std::endl;
        return 1;
    }
    std::cout << msh_errno << std::endl;
    return 0;
}
