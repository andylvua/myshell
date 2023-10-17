//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_MSH_BUILTIN_H
#define TEMPLATE_MSH_BUILTIN_H

#include "types/msh_command.h"
#include "types/msh_builtin_doc.h"

#include <map>
#include <string>
#include <cstring>
#include <iostream>

extern const std::map<std::string, command::func_t> internal_commands;

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

#endif //TEMPLATE_MSH_BUILTIN_H
