// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <fstream>

#include "internal/msh_utils.h"
#include "internal/msh_exec.h"
#include "internal/msh_parser.h"

#include "msh_external.h"

#include <sys/stat.h>

// Define GNU extension for execvpe on some systems
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

int exec_line_no = 0;
std::string exec_path;


/**
 * @brief Executes a script line by line.
 *
 * @warning Caller must ensure that the file exists and is readable.
 *
 * exec_path and exec_line_no are set for error_log().
 * @see error_log()
 *
 * @param path Path to the script.
 * @return Exit status of the script.
 *
 * @see msh_execve
 */
int msh_exec_script(const char *path) {
    std::ifstream script(path);
    std::string line;

    exec_path = path;
    exec_line_no = 0;

    while (std::getline(script, line)) {
        ++exec_line_no;
        auto command = parse_input(line);
        command.execute();
    }
    return 0;
}


/**
 * @brief Executes a command.
 *
 * @param argv Array of arguments.
 * @return Exit status of the command.
 *
 * If command contains a slash, it is executed directly, supposing it is a full path to the
 * executable. Otherwise, the command is searched in the PATH environment variable using execvpe().
 * If execve() fails and errno is ENOEXEC, the command is treated as a script and executed using
 * msh_exec_script().
 *
 * @see execve
 * @see execvpe
 * @see msh_exec_script
 */
int msh_execve(char **argv) {
    int status = 0;

    if (std::string(argv[0]).find('/') != std::string::npos) {
        execve(argv[0], argv, environ);
        if (errno == ENOEXEC) {
            msh_exec_script(argv[0]);
        } else {
            struct stat st{};
            if (stat(argv[0], &st) == 0 && S_ISDIR(st.st_mode)) {
                print_error(std::string(argv[0]) + ": Is a directory");
                status = UNKNOWN_ERROR;
                return status;
            } else {
                print_error(std::string(argv[0]) + ": " + strerror(errno));
                status = UNKNOWN_ERROR;
            }
        }
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
    return status;
}

/**
 * @brief Forks and executes a command using msh_execve().
 *
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Exit status of the command.
 *
 * @see fork
 * @see waitpid
 * @see msh_execve
 */
int msh_exec(int, char **argv) {
    int pid = fork();
    int status = 0;

    if (pid == 0) {
        exit(msh_execve(argv));
    } else if (pid < 0) {
        print_error(strerror(errno));
        status = UNKNOWN_ERROR;
    } else {
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }

    return status;
}
