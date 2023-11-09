//
// Created by andrew on 10/8/23.
//

#ifndef MYSHELL_MSH_ERROR_H
#define MYSHELL_MSH_ERROR_H

#include <string>

extern int msh_errno;

void error_log();

void msh_error(const std::string &msg);

enum msh_err {
    INTERNAL_ERROR = 1,
    COMMAND_NOT_FOUND = 127,
    UNKNOWN_ERROR = 128
};

#endif //MYSHELL_MSH_ERROR_H
