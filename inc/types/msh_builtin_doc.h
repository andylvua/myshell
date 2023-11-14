//
// Created by andrew on 10/13/23.
//

#ifndef MYSHELL_MSH_BUILTIN_DOC_H
#define MYSHELL_MSH_BUILTIN_DOC_H

#include <string>

/**
 * @brief Documentation for a built-in command.
 */
struct builtin_doc {
    std::string name;
    std::string args;
    std::string brief;
    std::string doc{};

    [[nodiscard]] std::string get_usage() const {
        return "Usage: " + name + " " + args;
    }
};

#endif //MYSHELL_MSH_BUILTIN_DOC_H
