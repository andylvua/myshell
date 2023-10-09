// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

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
    using func_t = int (*)(int argc, char **argv);

    std::string cmd;
    std::vector<std::string> argv;
    std::vector<char *> argv_c;
    int argc;
    func_t func;

    command() : cmd(), argv(), argv_c(), argc(0), func(nullptr) {}

    command(std::string cmd, std::vector<std::string> args, int (*func)(int argc, char **argv)) :
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
