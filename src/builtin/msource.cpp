// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"
#include "internal/msh_utils.h"
#include "internal/msh_exec.h"

#include <fstream>

static const builtin_doc doc = {
        .name   = "msource (or .)",
        .args   = "<file> [-h|--help]",
        .brief  = "Execute commands from a given file in the current shell.",
        .doc    = "Reads commands from a given file line by line and executes them in the current shell.\n"
                  "Returns 0 unless file can't be opened."
};

int msource(int argc, char **argv) {
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
        std::cerr << "msource: wrong number of arguments" << std::endl;
        std::cerr << doc.get_usage() << std::endl;
        return 1;
    }

    if (std::ifstream script(argv[1]); !script.good()) {
        std::cerr << "Couldn't open file " << argv[1] << std::endl;
        return 1;
    }

    msh_exec_script(argv[1]);
    return 0;
}
