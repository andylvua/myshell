//
// Created by andrew on 11/14/23.
//

#ifndef MYSHELL_MSH_BUILIN_COMMAND_H
#define MYSHELL_MSH_BUILIN_COMMAND_H

#include <string>

constexpr int DECLARATION_COMMAND = 1 << 0;

using builtin_func_t = int (*)(int, char **);

/**
 * @brief The builtin command structure.
 *
 * Contains the builtin function pointer and flags.
 */
struct builtin {
    builtin_func_t func;
    int flags;

    bool get_flag(int flag) const {
        return flags & flag;
    }
};

#endif //MYSHELL_MSH_BUILIN_COMMAND_H
