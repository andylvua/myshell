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
#include "types/msh_token.h"

std::string generate_prompt();

void set_variables(std::vector<Token> &tokens);

void expand_glob(std::vector<Token> &tokens);

void expand_vars(std::vector<Token> &tokens);

void squash_tokens(std::vector<Token> &tokens);

int check_syntax(std::vector<Token> &tokens);

std::vector<std::string> split_tokens(std::vector<Token> &tokens);

int process_tokens(std::vector<Token> &tokens);

#endif //TEMPLATE_UTILS_H
