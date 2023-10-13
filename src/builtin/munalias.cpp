//
// Created by andrew on 10/11/23.
//

#include "internal/msh_builtin.h"

static builtin_doc doc = {
        .args   = "munalias <alias>... [-h|--help]",
        .brief  = "Remove aliases.",
        .doc    = "Removes aliases from the list of aliases.\n"
                  "Returns 0 unless alias is not found or no arguments are given."
};

int munalias(int argc, char **argv) {
    if (handle_help(argc, argv, doc)) {
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
