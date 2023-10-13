// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/9/23.
//

#include "msh_external.h"

#include <string>
#include <boost/algorithm/string.hpp>

/**
 * @brief Check if given command is myshell shipped external program based on the EXTERNAL variable.
 * @param cmd Command to check.
 * @return True if command is external, false otherwise.
 *
 * @note EXTERNAL variable is set by the build system. The external programs should
 * be placed in {PROJECT_ROOT}/external directory to be automatically detected.
 *
 * @see EXTERNAL
 */
bool is_msh_external(const std::string &cmd) {
    std::string external(EXTERNAL);
    std::string delimiter = ";";

    std::vector<std::string> programs;
    boost::split(programs, external, boost::is_any_of(delimiter));

    return std::find(programs.begin(), programs.end(), cmd) != programs.end();
}
