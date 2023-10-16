// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/9/23.
//

#include "internal/msh_internal.h"
#include "types/msh_token.h"
#include "msh_external.h"
#include "msh_history.h"

#include <readline/history.h>

std::vector<variable> variables;

// MAYBE: Store token flags in the token itself. Could possibly be more clear and less error-prone
//  In that case, properly handle TokenType in constructors and/or on type change operations

std::map<TokenType, int> token_flags = {
        {TokenType::WORD,      WORD_LIKE},
        {TokenType::COMMAND,   WORD_LIKE | GLOB_NO_EXPAND},
        {TokenType::DQSTRING,  GLOB_NO_EXPAND | WORD_LIKE | IS_STRING},
        {TokenType::SQSTRING,  VAR_NO_EXPAND | GLOB_NO_EXPAND | WORD_LIKE | IS_STRING},
        {TokenType::VAR_DECL,  0},
        {TokenType::SUBOPEN,   UNSUPPORTED | COMMAND_SEPARATOR},
        {TokenType::SUBCLOSE,  UNSUPPORTED},
        {TokenType::COMMENT,   0},
        {TokenType::AMP,       UNSUPPORTED | COMMAND_SEPARATOR},
        {TokenType::AND,       UNSUPPORTED},
        {TokenType::PIPE,      UNSUPPORTED | COMMAND_SEPARATOR},
        {TokenType::OR,        UNSUPPORTED},
        {TokenType::OUT,       UNSUPPORTED},
        {TokenType::IN,        UNSUPPORTED},
        {TokenType::SEMICOLON, UNSUPPORTED | COMMAND_SEPARATOR},
};
/*NOTE:
 * For COMMAND, GLOB_NO_EXPAND flag is set due to requirement "We ignore masks in the program name."
 * However, this behavior is unnatural.
 */
/*MAYBE:
 * Remove GLOB_NO_EXPAND for DQSTRING tokens to satisfy "Substitution of masks (wildcard) in double quotes."
 * requirement. However, such behavior is unexpected too. Worth to discuss.
 */


/**
 * @brief Get a pointer to an internal variable with the given name.
 * @param name Name of the variable.
 * @return Pointer to the variable, @c nullptr otherwise
 */
variable *get_variable(const std::string &name) {
    for (auto &var : variables) {
        if (var.name == name) {
            return &var;
        }
    }
    return nullptr;
}

/**
 * @brief Set the value of an internal variable with the given name.
 * @param name Name of the variable.
 * @param value Value to set.
 */
void set_variable(const std::string &name, const std::string &value) {
    auto *var = get_variable(name);
    if (var == nullptr) {
        variables.emplace_back(name, value);
    } else {
        var->value = value;
    }
}

/**
 * @brief Initialize the shell.
 *
 * Copies the current process environment variables internally.
 * Sets the @c SHELL and @c VERSION to default values specified in msh_internal.h
 */
void msh_init() {
    atexit(msh_exit);
    read_history(MSH_HISTORY_PATH);

    extern char** environ;
    for (char** env = environ; *env != nullptr; ++env) {
        std::string env_string(*env);
        size_t pos = env_string.find('=');
        if (pos != std::string::npos) {
            std::string name = env_string.substr(0, pos);
            std::string value = env_string.substr(pos + 1);
            set_variable(name, value);
        }
    }

    set_variable("SHELL", SHELL);
    setenv("SHELL", SHELL, 1);
    set_variable("VERSION", VERSION);

    auto new_path = std::string(MSH_EXTERNAL_BIN_PATH) + ":";
    if (auto path = getenv("PATH"); path != nullptr) {
        new_path += path;
    }
    setenv("PATH", new_path.c_str(), 1);
    set_variable("PATH", new_path);

    // TODO! Read some .mshrc file and execute it to support setup scripts.
    //  Path should be provided by the build system or via config file by the user
}

/**
 * @brief Perform necessary operations before exiting the shell.
 *
 * Saves the history to the file specified by MSH_HISTORY_PATH.
 *
 * @note MSH_HISTORY_PATH is set automatically by the build system.
 *
 * @see MSH_HISTORY_PATH
 * @see write_history
 * @see atexit
 */
void msh_exit() {
    write_history(MSH_HISTORY_PATH);
}
