//
// Created by andrew on 10/8/23.
//

#ifndef MYSHELL_MSH_ERRNO_H
#define MYSHELL_MSH_ERRNO_H

extern int msh_errno;

enum msh_err {
    COMMAND_NOT_FOUND = 127,
    UNKNOWN_ERROR = 128
};

#endif //MYSHELL_MSH_ERRNO_H
