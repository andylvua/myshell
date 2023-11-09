//
// Created by andrew on 11/4/23.
//

#ifndef MYSHELL_MSH_COMMAND_FWD_H
#define MYSHELL_MSH_COMMAND_FWD_H

#include <memory>

struct command;
using simple_command_t = struct simple_command;
using connection_command_t = struct connection_command;
using simple_command_ptr = std::shared_ptr<simple_command_t>;
using connection_command_ptr = std::shared_ptr<connection_command_t>;

#endif //MYSHELL_MSH_COMMAND_FWD_H
