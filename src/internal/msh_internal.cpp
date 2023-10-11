//
// Created by andrew on 10/9/23.
//

#include "internal/msh_internal.h"
#include "types/msh_token.h"

std::vector<variable> variables;

std::map<TokenType, int> token_flags = {
        {TokenType::WORD,      WORD_LIKE},
        {TokenType::COMMAND,   WORD_LIKE},
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

// Yeeep, lots of unsupported shit happening here. To be completed later. God knows when tho...

variable *get_variable(const std::string &name) {
    for (auto &var : variables) {
        if (var.name == name) {
            return &var;
        }
    }
    return nullptr;
}

void set_variable(const std::string &name, const std::string &value) {
    auto *var = get_variable(name);
    if (var == nullptr) {
        variables.push_back({name, value});
    } else {
        var->value = value;
    }
}

void msh_init() {
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

    // TODO! Read some .mshrc file and execute it to support setup scripts
}
