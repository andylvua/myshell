//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>

int msh_errno = 0;

static char doc[] = "merrno -- Print the last error code.";

int merrno(int argc, char **argv) {
    if (handle_help(argc, argv, doc, nullptr)) {
        return 0;
    }

    if (argc > 1) {
        std::cout << "Usage: merrno" << std::endl;
        return 1;
    }
    std::cout << msh_errno << std::endl;
    return 0;
}
