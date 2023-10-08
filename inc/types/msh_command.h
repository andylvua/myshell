//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_MSH_COMMAND_H
#define TEMPLATE_MSH_COMMAND_H

#include "internal/msh_errno.h"

#include <string>
#include <vector>
#include <utility>
#include <iostream>

struct command {
    std::string cmd;
    std::vector<std::string> argv;
    std::vector<char *> argv_c;
    int argc;

    int (*func)(int argc, char **argv);

    command(std::string cmd, std::vector<std::string> args, int (*func)(int argc, char **argv)):
            cmd(std::move(cmd)), argv(std::move(args)), func(func) {
        argc = static_cast<int>(this->argv.size());
        for (auto &arg: argv) {
            argv_c.push_back(arg.data());
        }
        argv_c.push_back(nullptr);
    }

    void execute() const {
        if (func == nullptr) {
            return;
        }
        msh_errno = func(argc, const_cast<char **>(cargv()));
    }

    [[nodiscard]] char *const *cargv() const {
        return &argv_c[0];
    }

    [[maybe_unused]] void print() const {
        std::cout << "command: ";
        std::cout << cmd << std::endl;
        std::cout << "argv: " << std::endl;
        for (auto &arg: argv) {
            std::cout << arg << std::endl;
        }
    }
};


#endif //TEMPLATE_MSH_COMMAND_H
