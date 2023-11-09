//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_UTILS_H
#define TEMPLATE_UTILS_H

#include "types/msh_command_fwd.h"
#include "types/msh_token.h"

#include <iostream>
#include <map>
#include <string>
#include <vector>


std::string generate_prompt();

void set_variables(tokens_t &tokens);

void expand_aliases(tokens_t &tokens);

void expand_glob(tokens_t &tokens);

void expand_vars(tokens_t &tokens);

void squash_tokens(tokens_t &tokens);

void process_tokens(tokens_t &tokens);

void check_syntax(const tokens_t &tokens);

simple_command_ptr make_simple_command(const tokens_t &tokens);

command split_commands(const tokens_t &tokens);

#endif //TEMPLATE_UTILS_H
