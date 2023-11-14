// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/9/23.
//
/**
 * @file
 * @brief Internal utilities.
 */

#include "types/msh_token.h"
#include "msh_external.h"
#include "msh_history.h"
#include "internal/msh_jobs.h"
#include "internal/msh_internal.h"

#include <cstdio>
#include <readline/history.h>

/**
 * @brief The internal variable table.
 *
 * Maps variable names to variable objects.
 *
 * @see variable
 */
std::vector<variable> variables;

/**
 * @brief The internal token flags table.
 *
 * Maps token types to their corresponding flags.
 *
 * @see TokenType
 *
 * @note Wildcard expansion is disabled for double quoted strings by default.
 * To enable it, define ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION in CMakeLists.txt.
 *
 * @note Glob expansion is not performed for COMMAND tokens by default due to requirement
 * `We ignore masks in the program name.` However, this behavior is unnatural.
 */
const std::map<TokenType, int> token_flags = {
        {TokenType::EMPTY,     0},
        {TokenType::WORD,      WORD_LIKE | GLOB_EXPAND | VAR_EXPAND},
        {TokenType::COMMAND,   WORD_LIKE | VAR_EXPAND},
#ifdef ENABLE_DOUBLE_QUOTE_WILDCARD_SUBSTITUTION
        {TokenType::DQSTRING,  WORD_LIKE | IS_STRING | VAR_EXPAND | GLOB_EXPAND},
#else
        {TokenType::DQSTRING, WORD_LIKE | NO_WORD_SPLIT | VAR_EXPAND},
#endif
        {TokenType::SQSTRING, WORD_LIKE | NO_WORD_SPLIT},
        {TokenType::VAR_DECL,  0},
        {TokenType::SUBOPEN,   UNSUPPORTED | COMMAND_SEPARATOR},
        {TokenType::SUBCLOSE,  UNSUPPORTED},
        {TokenType::AMP,       COMMAND_SEPARATOR},
        {TokenType::AND,       COMMAND_SEPARATOR},
        {TokenType::PIPE,      COMMAND_SEPARATOR},
        {TokenType::PIPE_AMP,  COMMAND_SEPARATOR},
        {TokenType::OR,        COMMAND_SEPARATOR},
        {TokenType::OUT,       REDIRECT},
        {TokenType::OUT_APPEND,REDIRECT},
        {TokenType::IN,        REDIRECT},
        {TokenType::OUT_AMP,   REDIRECT},
        {TokenType::IN_AMP,    REDIRECT},
        {TokenType::AMP_OUT,   REDIRECT},
        {TokenType::AMP_APPEND,REDIRECT},
        {TokenType::SEMICOLON, COMMAND_SEPARATOR},
        {TokenType::COM_SUB,   WORD_LIKE},
};

/**
 * @brief Get a pointer to an internal variable with the given name.
 *
 * @param name Name of the variable.
 * @return Pointer to the variable, @c nullptr otherwise
 */
variable *get_variable(std::string_view name) {
    for (auto &var : variables) {
        if (var.name == name) {
            return &var;
        }
    }
    return nullptr;
}

/**
 * @brief Set the value of an internal variable with the given name.
 *
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
 * This function should be called before any other shell functions.
 *
 * Sets up necessary handlers, job control, and copies the current process
 * environment variables internally.
 *
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

    init_job_control();
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
