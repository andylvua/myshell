//
// Created by andrew on 10/9/23.
//

#ifndef MYSHELL_MSH_TOKEN_H
#define MYSHELL_MSH_TOKEN_H

#include <string>
#include <utility>
#include <map>

enum TokenType {
    EMPTY,
    WORD,
    AMP,
    AND,
    PIPE,
    OR,
    OUT,
    IN,
    SEMICOLON,
    DQSTRING,
    SQSTRING,
    COMMENT,
    VAR_DECL,
    SUBOPEN,
    SUBCLOSE
};

extern std::map<TokenType, int> token_flags;

constexpr int UNSUPPORTED = 1 << 0; // Unsupported token
constexpr int GLOB_NO_EXPAND = 1 << 1; // Don't expand globs
constexpr int VAR_NO_EXPAND = 1 << 2; // Don't expand variables
constexpr int WORD_LIKE = 1 << 3; // Word-like token

struct Token {
    TokenType type;
    std::string value;
    char open_until;

    explicit Token(TokenType t = EMPTY, char openUntil = '\0') : type(t), open_until(openUntil) {}

    Token(TokenType t, std::string value, char openUntil = '\0') : type(t), value(std::move(value)),
                                                                   open_until(openUntil) {}
};

#endif //MYSHELL_MSH_TOKEN_H
