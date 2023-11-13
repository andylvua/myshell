//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_MSH_EXEC_H
#define TEMPLATE_MSH_EXEC_H

#include "types/msh_command_fwd.h"

#include <unistd.h>


extern int exec_line_no;
extern std::string exec_path;

constexpr int BUILTIN = 1 << 0;
constexpr int FORK_NO_WAIT = 1 << 1;
constexpr int ASYNC = 1 << 2;
constexpr int FORCE_PIPE = 1 << 3;


int msh_exec_script(const char *path);

int msh_execve(char **argv);

int msh_exec_simple(simple_command &cmd, int pipe_in, int pipe_out, int flags);

int msh_exec_internal(command &cmd, int in = STDIN_FILENO, int out = STDOUT_FILENO, int flags = 0);

#endif //TEMPLATE_MSH_EXEC_H
