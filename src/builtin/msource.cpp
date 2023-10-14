// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"
#include "internal/msh_utils.h"
#include "internal/msh_exec.h"

#include <fstream>

static builtin_doc doc = {
        .args   = "msource (or .) <file> [-h|--help]",
        .brief  = "Execute commands from a given file in the current shell.",
        .doc    = "Reads commands from a given file line by line and executes them in the current shell.\n"
                  "Returns 0 unless file can't be opened."
};

int msource(int argc, char **argv) {
    if (handle_help(argc, argv, doc)) {
        return 0;
    }

    std::ifstream script(argv[1]);
    if (!script.good()) {
        std::cout << "Couldn't open file " << argv[1] << std::endl;
        return 1;
    }

    msh_exec_script(argv[1]);
    return 0;
}
