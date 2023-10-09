// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>

static char doc[] = "mexit -- Exit the shell with a status of N. If N is omitted, the exit status is 0.";
static char args_doc[] = "<N>";

int mexit(int argc, char **argv) {
    if (handle_help(argc, argv, doc, args_doc)) {
        return 0;
    }

    if (argc == 1) {
        exit(0);
    }
    if (argc > 2) {
        std::cout << "Usage: mexit <code>" << std::endl;
        return 1;
    }
    exit(std::stoi(argv[1]));
}
