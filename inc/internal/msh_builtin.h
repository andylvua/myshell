//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_MSH_BUILTIN_H
#define TEMPLATE_MSH_BUILTIN_H

#include "internal/msh_error.h"
#include "types/msh_builtin_doc.h"
#include "types/msh_builin_command.h"

#include <map>
#include <string>
#include <cstring>
#include <iostream>

using func_t = int (*)(int, char **);

extern const std::map<std::string, builtin> builtin_commands;

extern std::map<std::string, std::string> aliases;

bool handle_help(int argc, char **argv, const builtin_doc &doc);

bool is_builtin(const std::string &cmd);

int merrno(int argc, char **argv);

int mpwd(int argc, char **argv);

int mcd(int argc, char **argv);

int mexit(int argc, char **argv);

int mecho(int argc, char **argv);

int mexport(int argc, char **argv);

int msource(int argc, char **argv);

int malias(int argc, char **argv);

int munalias(int argc, char **argv);

int mjobs(int argc, char **argv);

#endif //TEMPLATE_MSH_BUILTIN_H
