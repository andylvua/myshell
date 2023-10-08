//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_UTILS_H
#define TEMPLATE_UTILS_H

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "types/msh_command.h"

std::string prompt();

void print_error(const std::string &msg);

void expand_glob(std::vector<std::string> &args);

void expand_vars(std::vector<std::string> &args);

void strip_comments(std::string &input);

command parse_input(std::string input);

#endif //TEMPLATE_UTILS_H
