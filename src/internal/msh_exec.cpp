// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <cstring>

#include "internal/msh_utils.h"
#include "internal/msh_exec.h"

#include "msh_external.h"

// Define GNU extension for execvpe on some systems
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


/**
 * @brief Forks and executes a command.
 *
 * If the command contains a slash, it is executed directly,
 * meaning that user specified a path to the program. Otherwise, the command
 * is searched in the PATH environment variable by execvpe.
 *
 * If the program is shipped with myshell, its path is determined based on the
 * MSH_EXTERNAL_BIN_PATH variable set by the build system.
 *
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Exit status of the command.
 *
 * @see is_msh_external
 * @see MSH_EXTERNAL_BIN_PATH
 * @see execvpe
 * @see execve
 * @see fork
 * @see waitpid
 */
int fork_exec(int, char **argv) {
    int pid = fork();
    int status = 0;

    if (pid == 0) {
        if (std::string(argv[0]).find('/') != std::string::npos) {
            execve(argv[0], argv, environ);
            print_error(std::string(argv[0]) + ": " + strerror(errno));
            status = UNKNOWN_ERROR;
        } else {
            execvpe(argv[0], argv, environ);
            if (errno == ENOENT) {
                print_error("Command not found: " + std::string(argv[0]));
                status = COMMAND_NOT_FOUND;
            } else {
                print_error(strerror(errno));
                status = UNKNOWN_ERROR;
            }
        }
        exit(status);
    } else if (pid < 0) {
        print_error(strerror(errno));
        status = UNKNOWN_ERROR;
    } else {
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }

    return status;
}
