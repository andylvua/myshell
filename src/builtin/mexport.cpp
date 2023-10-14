// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"
#include "internal/msh_internal.h"

#include <iostream>
#include <unistd.h>

int mexport(int argc, char **argv) {
    if (argc == 1) {
        for (int i = 0; environ[i] != nullptr; i++) {
            std::cout << environ[i] << std::endl;
        }
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        auto arg = std::string(argv[i]);

        auto pos = arg.find('=');
        if (pos == std::string::npos) {
            continue;
        }

        auto var = arg.substr(0, pos);
        auto value = arg.substr(pos + 1);
        if (setenv(var.data(), value.data(), 1) != 0) {
            std::cout << "Couldn't set environment variable " << var << std::endl;
            return 1;
        }
        set_variable(var, value);
    }
    return 0;
}
