//
// Created by andrew on 10/9/23.
//

#ifndef MYSHELL_MSH_TOKEN_H
#define MYSHELL_MSH_TOKEN_H

#include <string>
#include <utility>
#include <map>
#include <vector>

struct Token;
using tokens_t = std::vector<Token>;

enum class TokenType {
    EMPTY, // TODO! Properly handle <newline> <carriage return> etc.
    WORD,
    COMMAND,
    AMP,
    AND,
    PIPE,
    OR,
    OUT,
    OUT_APPEND,
    IN,
    OUT_AMP,
    IN_AMP,
    AMP_OUT,
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
constexpr int REDIRECT = 1 << 6; // This token is a redirect. Used for parsing

struct Token {
    TokenType type;
    std::string value;
    char open_until;
    int flags = 0;

    explicit Token(TokenType t = TokenType::EMPTY, char openUntil = '\0') : type(t), open_until(openUntil) {
        flags = token_flags.at(type);
    }

    Token(TokenType t, std::string value, char openUntil = '\0') : type(t), value(std::move(value)),
                                                                   open_until(openUntil) {
        flags = token_flags.at(type);
    }

    void set_type(TokenType t) {
        type = t;
        flags = token_flags.at(type);
    }

    [[nodiscard]] bool get_flag(int flag) const {
        return flags & flag;
    }

    [[maybe_unused]] void set_flag(int flag) {
        flags |= flag;
    }

    [[maybe_unused]] void unset_flag(int flag) {
        flags &= ~flag;
    }
};

#endif //MYSHELL_MSH_TOKEN_H
