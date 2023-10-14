// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/14/23.
//

#include "internal/msh_error.h"
#include "internal/msh_exec.h"

#include <iostream>

/**
 * @brief Log the line number and path of the executed script, if any.
 *
 * @see print_error()
 */
void error_log() {
    if (exec_line_no > 0) {
        std::cerr << exec_path << ":" << exec_line_no << ": ";
    }
}

/**
 * @brief Print an error message to stderr with a `myshell: ` prefix.
 *
 * Before printing the message, error_log() is called to log the line number and path of the
 * executed script, if any.
 * @see error_log()
 *
 * @param msg Error message.
 */
void print_error(const std::string &msg) {
    error_log();
    std::cerr << "myshell: " << msg << std::endl;
}