//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"

#include <iostream>

static char doc[] = "mecho -- Write arguments to the standard output.";
static char args_doc[] = "<string>...";

int mecho(int argc, char **argv) {
    if (handle_help(argc, argv, doc, args_doc)) {
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}
