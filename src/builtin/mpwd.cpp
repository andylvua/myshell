//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>
#include <cstring>
#include <unistd.h>

static char doc[] = "mpwd -- Print the current working directory.";

int mpwd(int argc, char **argv) {
    if (handle_help(argc, argv, doc, nullptr)) {
        return 0;
    }

    if (argc > 1) {
        std::cout << "Usage: mpwd" << std::endl;
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
