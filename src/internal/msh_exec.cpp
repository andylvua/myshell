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

int fork_exec(int, char **argv) {
    int pid = fork();
    int status;

    if (pid == 0) {
        if (is_msh_external(argv[0])) {
            std::string path = std::string(MSH_EXTERNAL_BIN_PATH) + "/" + argv[0];
            execve(path.c_str(), argv, environ);
        } else {
            execvpe(argv[0], argv, environ);
        }
        if (errno == ENOENT) {
            print_error("command not found: " + std::string(argv[0]));
            status = COMMAND_NOT_FOUND;
        } else {
            print_error(strerror(errno));
            status = UNKNOWN_ERROR;
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
