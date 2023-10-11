//
// Created by andrew on 10/11/23.
//

#include "internal/msh_builtin.h"

static char doc[] = "malias -- Define or show aliases.";
static char args_doc[] = "[name[=value]]...";

int malias(int argc, char **argv) {
    if (handle_help(argc, argv, doc, args_doc)) {
        return 0;
    }

    if (argc == 1) {
        for (auto &alias : aliases) {
            std::cout << "alias " << alias.first << "=" << "'" << alias.second << "'" << std::endl;
        }
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        auto arg = std::string(argv[i]);
        auto pos = arg.find('=');
        if (pos == std::string::npos) {
            if (aliases.find(arg) != aliases.end()) {
                std::cout << "alias " << arg << "=" << "'" << aliases[arg] << "'" << std::endl;
            } else {
                std::cout << "alias " << arg << " not found" << std::endl;
                return 1;
            }
        } else {
            auto name = arg.substr(0, pos);
            auto value = arg.substr(pos + 1);
            aliases[name] = value;
        }
    }

    return 0;
}
