//
// Created by andrew on 10/9/23.
//

#ifndef MYSHELL_MSH_TOKEN_H
#define MYSHELL_MSH_TOKEN_H

#include <string>
#include <utility>
#include <map>

enum class TokenType {
    EMPTY, // TODO! Properly handle <newline> <carriage return> etc.
    WORD,
    COMMAND,
    AMP,
    AND,
    PIPE,
    OR,
    OUT,
    IN,
    SEMICOLON,
    DQSTRING,
    SQSTRING,
    VAR_DECL,
    SUBOPEN,
    SUBCLOSE
};

extern const std::map<TokenType, int> token_flags;

constexpr int UNSUPPORTED = 1 << 0; // Unsupported token. Parser will throw an error if it encounters this
constexpr int GLOB_EXPAND = 1 << 1; // Tells the parser to expand globs in this kind of token
constexpr int VAR_EXPAND = 1 << 2; // Tells the parser to expand variables in this kind of token
constexpr int WORD_LIKE = 1 << 3; // This token is a potential argument/command that will be forwarded to argv
constexpr int IS_STRING = 1 << 4; // This token is a string literal
constexpr int COMMAND_SEPARATOR = 1 << 5; // This token separates simple commands. Used for parsing

struct Token {
    TokenType type;
    std::string value;
    char open_until;

    explicit Token(TokenType t = TokenType::EMPTY, char openUntil = '\0') : type(t), open_until(openUntil) {}

    Token(TokenType t, std::string value, char openUntil = '\0') : type(t), value(std::move(value)),
                                                                   open_until(openUntil) {}

    [[nodiscard]] bool get_flag(int flag) const {
        return token_flags.at(type) & flag;
    }
};

#endif //MYSHELL_MSH_TOKEN_H