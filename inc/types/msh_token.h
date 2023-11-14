//
// Created by andrew on 10/9/23.
//
/**
 * @file
 * @brief Token structure and related types.
 */

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
    PIPE_AMP,
    OR,
    OUT,
    OUT_APPEND,
    IN,
    OUT_AMP,
    IN_AMP,
    AMP_OUT,
    AMP_APPEND,
    SEMICOLON,
    DQSTRING,
    SQSTRING,
    VAR_DECL,
    SUBOPEN,
    SUBCLOSE,
    COM_SUB,
};

extern const std::map<TokenType, int> token_flags;

/**
 * @defgroup token_flags Token flags
 *
 * These flags are used to tell the parser and utilities how to treat a specific token.
 *
 * @{
 */
constexpr int UNSUPPORTED = 1 << 0; ///< Unsupported token. Parser will throw an error if it encounters this
constexpr int GLOB_EXPAND = 1 << 1; ///< Tells the parser to expand globs in this kind of token
constexpr int VAR_EXPAND = 1 << 2; ///< Tells the parser to expand variables in this kind of token
constexpr int WORD_LIKE = 1 << 3; ///< This token is a potential argument/command that will be forwarded to argv
constexpr int NO_WORD_SPLIT = 1 << 4; ///< This token is not eligible for word splitting
constexpr int COMMAND_SEPARATOR = 1 << 5; ///< This token separates simple commands. Used for parsing
constexpr int REDIRECT = 1 << 6; ///< This token is a redirect. Used for parsing
/** @} */

/**
 * @brief Structure representing a token.
 *
 * Token is a single unit of input produced by the lexer and used throughout the shell.
 * All processing operations are performed on tokens.
 *
 * @see tokens_t
 * @see TokenType
 * @see token_flags
 */
struct Token {
    TokenType type = TokenType::EMPTY;
    std::string value;
    int flags = 0;

    Token() = default;

    explicit Token(TokenType t) : type(t) {
        flags = token_flags.at(type);
    }

    Token(TokenType t, std::string value) : type(t), value(std::move(value)) {
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
