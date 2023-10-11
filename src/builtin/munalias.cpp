//
// Created by andrew on 10/11/23.
//

#include "internal/msh_builtin.h"

static char doc[] = "munalias -- Remove aliases.";
static char args_doc[] = "<alias>...";

int munalias(int argc, char **argv) {
    if (handle_help(argc, argv, doc, args_doc)) {
        return 0;
    }

    if (argc == 1) {
        std::cout << "Usage: munalias <alias>" << std::endl;
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        auto arg = std::string(argv[i]);
        if (aliases.find(arg) != aliases.end()) {
            aliases.erase(arg);
        } else {
            std::cout << "alias " << arg << " not found" << std::endl;
            return 1;
        }
    }

    return 0;
}
