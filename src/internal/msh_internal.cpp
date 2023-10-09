//
// Created by andrew on 10/9/23.
//

#include "internal/msh_internal.h"
#include "types/msh_token.h"

std::vector<variable> variables;

std::map<TokenType, int> token_flags = {
        {TokenType::WORD,      WORD_LIKE},
        {TokenType::DQSTRING,  GLOB_NO_EXPAND | WORD_LIKE},
        {TokenType::SQSTRING,  VAR_NO_EXPAND | GLOB_NO_EXPAND | WORD_LIKE},
        {TokenType::VAR_DECL,  0},
        {TokenType::SUBOPEN,   UNSUPPORTED},
        {TokenType::SUBCLOSE,  UNSUPPORTED},
        {TokenType::COMMENT,   0},
        {TokenType::AMP,       UNSUPPORTED},
        {TokenType::AND,       UNSUPPORTED},
        {TokenType::PIPE,      UNSUPPORTED},
        {TokenType::OR,        UNSUPPORTED},
        {TokenType::OUT,       UNSUPPORTED},
        {TokenType::IN,        UNSUPPORTED},
        {TokenType::SEMICOLON, UNSUPPORTED},
};

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
