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
    COMMENT,
    VAR_DECL,
    SUBOPEN,
    SUBCLOSE
};

extern std::map<TokenType, int> token_flags;

constexpr int UNSUPPORTED = 1 << 0; // Unsupported token. Parser will throw an error if it encounters this
constexpr int GLOB_NO_EXPAND = 1 << 1; // Tells the parser not to expand globs in this kind of token
constexpr int VAR_NO_EXPAND = 1 << 2; // Tells the parser not to expand variables in this kind of token
constexpr int WORD_LIKE = 1 << 3; // This token is a potential argument or command, not interpreted by the shell
constexpr int IS_STRING = 1 << 4; // This token is a string literal
constexpr int COMMAND_SEPARATOR = 1 << 5; // This token separates simple commands. Used for parsing

struct Token {
    TokenType type;
    std::string value;
    char open_until;

    explicit Token(TokenType t = EMPTY, char openUntil = '\0') : type(t), open_until(openUntil) {}

    Token(TokenType t, std::string value, char openUntil = '\0') : type(t), value(std::move(value)),
                                                                   open_until(openUntil) {}
};

#endif //MYSHELL_MSH_TOKEN_H
