// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
//
// Created by andrew on 10/11/23.
//

#include "internal/msh_builtin.h"

static const builtin_doc doc = {
        .name   = "munalias",
        .args   = "<alias>... [-h|--help]",
        .brief  = "Remove aliases.",
        .doc    = "Removes aliases from the list of aliases.\n"
                  "Returns 0 unless alias is not found or no arguments are given."
};

int munalias(int argc, char **argv) {
    try {
        if (handle_help(argc, argv, doc)) {
            return 0;
        }
    } catch (std::exception &e) {
        print_error(doc.name + ": " + e.what());
        std::cerr << "Usage: " << doc.name << " " << doc.args << std::endl;
        return 1;
    }

    if (argc == 1) {
        std::cerr << "munalias: wrong number of arguments" << std::endl;
        std::cerr << doc.get_usage() << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        auto arg = std::string(argv[i]);
        if (aliases.contains(arg)) {
            aliases.erase(arg);
        } else {
            std::cerr << "alias " << arg << " not found" << std::endl;
            return 1;
        }
    }

    return 0;
}
