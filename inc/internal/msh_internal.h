//
// Created by andrew on 10/9/23.
//

#ifndef MYSHELL_MSH_INTERNAL_H
#define MYSHELL_MSH_INTERNAL_H

#include "types/msh_variable.h"

#include <vector>

constexpr auto SHELL = "msh";
constexpr auto VERSION = "2.0.0";

extern std::vector<variable> variables;

variable *get_variable(std::string_view name);

void set_variable(const std::string &name, const std::string &value);

void msh_init();

void msh_exit();

#endif //MYSHELL_MSH_INTERNAL_H
