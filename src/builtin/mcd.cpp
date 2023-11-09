// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>
#include <unistd.h>

static const builtin_doc doc = {
        .name   = "mcd",
        .args   = "<path> [-h|--help]",
        .brief  = "Change working directory",
        .doc    = "Returns 0 unless given wrong number of arguments or chdir() fails."
};


int mcd(int argc, char **argv) {
    try {
        if (handle_help(argc, argv, doc)) {
            return 0;
        }
    } catch (const std::exception &e) {
        msh_error(doc.name + ": " + e.what());
        std::cerr << "Usage: " << doc.name << " " << doc.args << std::endl;
        return 1;
    }

    if (argc != 2) {
        std::cerr << "cd: wrong number of arguments" << std::endl;
        std::cerr << doc.get_usage() << std::endl;
        return 1;
    }

    if (chdir(argv[1]) != 0) {
        std::cerr << "Couldn't change directory to " << argv[1] << std::endl;
        return 1;
    }
    return 0;
}
