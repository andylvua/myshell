// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "internal/msh_utils.h"
#include "internal/msh_exec.h"
#include "internal/msh_parser.h"
#include "internal/msh_jobs.h"

#include <unistd.h>
#include <cstring>
#include <fstream>
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
                msh_error(std::string(argv[0]) + ": Is a directory");
                status = UNKNOWN_ERROR;
            } else {
                msh_error(std::string(argv[0]) + ": " + strerror(errno));
                status = UNKNOWN_ERROR;
            }
        }
    } else {
        execvpe(argv[0], argv, environ);
        if (errno == ENOENT) {
            msh_error("Command not found: " + std::string(argv[0]));
            status = COMMAND_NOT_FOUND;
        } else {
            msh_error(strerror(errno));
            status = UNKNOWN_ERROR;
        }
    }
    return status;
}

int msh_exec_simple(simple_command &cmd, int pipe_in = STDIN_FILENO, int pipe_out = STDOUT_FILENO, int flags = 0) {
    int status = 0;
    bool to_fork;
    bool is_builtin = flags & BUILTIN;
    bool is_async = flags & ASYNC;

    to_fork = pipe_in != STDIN_FILENO || pipe_out != STDOUT_FILENO || !is_builtin || is_async;

    if (!to_fork) {
        std::vector<int> fd_to_close;
        if (auto res = cmd.do_redirects(&fd_to_close); res != 0) {
            return res;
        }
        status = builtin_commands.at(cmd.argv[0])(cmd.argc, cmd.argv_c.data());
        cmd.undo_redirects(fd_to_close);
        return status;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (pipe_in != STDIN_FILENO) {
            dup2(pipe_in, STDIN_FILENO);
            close(pipe_in);
        }
        if (pipe_out != STDOUT_FILENO) {
            dup2(pipe_out, STDOUT_FILENO);
            close(pipe_out);
        }
        if (auto res = cmd.do_redirects(nullptr); res != 0) {
            exit(res);
        }

        if (is_builtin) {
            status = builtin_commands.at(cmd.argv[0])(cmd.argc, cmd.argv_c.data());
        } else {
            status = msh_execve(cmd.argv_c.data());
        }
        exit(status);
    } else if (pid < 0) {
        msh_error(strerror(errno));
        return UNKNOWN_ERROR;
    } else {
        add_process(pid, flags, cmd.argv);

        if (is_async) {
            std::cout << "[" << no_background_processes() << "] " << pid << std::endl;
            return status;
        }
        if (flags & FORK_NO_WAIT) {
            return status;
        }

        return wait_for_process(pid, &status);
    }
}

int msh_exec_internal(command &cmd, int in, int out, int flags) {
    return std::visit([&in, &out, &flags](auto &&arg) -> int {
        return arg->execute(in, out, flags);
    }, cmd.cmd);
}
