// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/11/23.
//

#include "internal/msh_builtin.h"

static builtin_doc doc = {
        .args   = "malias [name[=value] ...] [-h|--help]",
        .brief  = "Create or print aliases",
        .doc    = "Without arguments, prints all aliases.\n\n"
                  "If arguments are given, creates an alias for each argument of the form NAME=VALUE\n"
                  "or prints the value of the alias with the given name.\n\n"
                  "Returns 0 unless an unknown alias is given."
};

int malias(int argc, char **argv) {
    if (handle_help(argc, argv, doc)) {
        return 0;
    }

    if (argc == 1) {
        for (auto &alias: aliases) {
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
            aliases[name] = std::move(value);
        }
    }

    return 0;
}
