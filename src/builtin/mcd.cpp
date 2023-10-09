// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>
#include <unistd.h>

static char doc[] = "mcd -- Change the shell working directory.";
static char args_doc[] = "<path>";

int mcd(int argc, char **argv) {
    if (handle_help(argc, argv, doc, args_doc)) {
        return 0;
    }

    if (argc != 2) {
        std::cout << "cd: wrong number of arguments" << std::endl;
        return 1;
    }

    if (chdir(argv[1]) != 0) {
        std::cout << "Couldn't change directory to " << argv[1] << std::endl;
        return 1;
    }
    return 0;
}
