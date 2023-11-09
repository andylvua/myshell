// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>
#include <cstring>
#include <unistd.h>

static const builtin_doc doc = {
        .name   = "mpwd",
        .args   = "[-h|--help]",
        .brief  = "Print the current working directory",
        .doc    = "Returns 1 if any arguments specified or getcwd() fails, 0 otherwise."
};

int mpwd(int argc, char **argv) {
    try {
        if (handle_help(argc, argv, doc)) {
            return 0;
        }
    } catch (const std::exception &e) {
        msh_error(doc.name + ": " + e.what());
        std::cerr << "Usage: " << doc.name << " " << doc.args << std::endl;
        return 1;
    }

    if (argc > 1) {
        std::cerr << "mpwd: wrong number of arguments" << std::endl;
        std::cerr << doc.get_usage() << std::endl;
        return 1;
    }
    char *cwd = getcwd(nullptr, 0);
    if (cwd == nullptr) {
        std::cerr << "Error: " << strerror(errno) << std::endl;
        return 1;
    }
    std::cout << cwd << std::endl;
    free(cwd);
    return 0;
}
