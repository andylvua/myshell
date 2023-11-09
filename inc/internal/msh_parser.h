//
// Created by andrew on 10/9/23.
//

#ifndef MYSHELL_MSH_PARSER_H
#define MYSHELL_MSH_PARSER_H

#include "types/msh_command.h"
#include "types/msh_token.h"

#include <string>
#include <vector>

tokens_t lexer(const std::string &input);

command parse_input(std::string input);

#endif //MYSHELL_MSH_PARSER_H
