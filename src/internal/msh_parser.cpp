// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/9/23.
//

#include "internal/msh_parser.h"
#include "internal/msh_utils.h"
#include "internal/msh_builtin.h"

#include <boost/algorithm/string.hpp>

/**
 * Perform lexical analysis on the given input string, breaking it down into a vector of tokens.
 *
 * @param input The input string to be analyzed.
 * @return A vector of Token objects.
 */
tokens_t lexer(const std::string &input) {
    using enum TokenType;

    tokens_t tokens;
    Token currentToken;
    bool is_quotes = false, command_expected = true;
    char current_char, next_char;
    size_t i = 0, len = input.length();

    while (i < len) {
        current_char = input[i];
        next_char = i + 1 < len ? input[i + 1] : '\0';

        if (!tokens.empty() && tokens.back().type == WORD && command_expected) {
            tokens.back().set_type(COMMAND);
            command_expected = false;
        }
        command_expected |= currentToken.get_flag(COMMAND_SEPARATOR);

        if (current_char == '\'' || current_char == '\"') {
            is_quotes = !is_quotes;
        }

        if (currentToken.open_until) {
            if (current_char == currentToken.open_until) {
                currentToken.open_until = '\0';
            } else if (current_char == '\\' && next_char == '\\') {
                currentToken.value += current_char;
                ++i;
            } else if (current_char == '\\' && next_char == '"' && currentToken.open_until == '"') {
                currentToken.value += next_char;
                ++i;
            } else {
                currentToken.value += current_char;
            }
            ++i;
            continue;
        }

        switch (current_char) {
            case '\\':
                if (currentToken.type != WORD) {
                    tokens.push_back(currentToken);
                    currentToken = Token(WORD);
                }
                currentToken.value += next_char;
                i++;
                break;
            case '&':
                if (currentToken.type == AMP) currentToken.set_type(AND);
                else {
                    tokens.push_back(currentToken);
                    if (next_char == '>') {
                        currentToken = Token(AMP_OUT, "&>");
                        ++i;
                    } else {
                        currentToken = Token(AMP, "&");
                    }
                }
                break;
            case '|':
                if (currentToken.type == PIPE) currentToken.set_type(OR);
                else {
                    tokens.push_back(currentToken);
                    currentToken = Token(PIPE, "|");
                }
                break;
            case '>':
                if (currentToken.type == OUT) currentToken.set_type(OUT_APPEND);
                else {
                    tokens.push_back(currentToken);
                    if (next_char == '&') {
                        currentToken = Token(OUT_AMP, ">&");
                        ++i;
                    } else {
                        currentToken = Token(OUT, ">");
                    }
                }
                break;
            case '<':
                tokens.push_back(currentToken);
                if (next_char == '&') {
                    currentToken = Token(IN_AMP, "<&");
                    ++i;
                } else {
                    currentToken = Token(IN, "<");
                }
                break;
            case ';':
                tokens.push_back(currentToken);
                currentToken = Token(SEMICOLON, ";");
                break;
            case '\"':
                tokens.push_back(currentToken);
                currentToken = Token(DQSTRING, '\"');
                break;
            case '\'':
                tokens.push_back(currentToken);
                currentToken = Token(SQSTRING, '\'');
                break;
            case '=':
                if (!is_quotes && command_expected) {
                    currentToken.set_type(VAR_DECL);
                }
                if (currentToken.type == EMPTY) {
                    tokens.push_back(currentToken);
                    currentToken = Token(WORD);
                }
                currentToken.value += current_char;
                break;
            case '#':
                if (!is_quotes) {
                    tokens.push_back(currentToken);
                    while (i < len && input[i] != '\n') {
                        i++;
                    }
                }
                break;
            case '(':
                tokens.push_back(currentToken);
                currentToken = Token(SUBOPEN, "(");
                break;
            case ')':
                tokens.push_back(currentToken);
                currentToken = Token(SUBCLOSE, ")");
                break;
            case ' ':
                tokens.push_back(currentToken);
                currentToken = Token(EMPTY);
                break;
            default:
                if (currentToken.type == WORD || currentToken.type == VAR_DECL) {
                    currentToken.value += current_char;
                } else {
                    tokens.push_back(currentToken);
                    currentToken = Token(WORD);
                    currentToken.value = current_char;
                }
        }
        i++;
    }

    tokens.push_back(currentToken);
    tokens.erase(tokens.begin());
    if (!tokens.empty() && tokens.back().type == WORD && command_expected) {
        tokens.back().set_type(COMMAND);
    }

    return tokens;
}

/**
 * @brief Parse the input string to prepare it for command execution.
 *
 * Performs lexical analysis
 * to tokenize the input, and checks for validity. If the input is valid, it prepares the arguments and
 * determines the execution function.
 *
 * @param input The input string to be parsed.
 * @return A @c command object containing the arguments and execution function.
 *
 * @see lexer()
 * @see process_tokens()
 * @see make_simple_command()
 * @see msh_exec_simple()
 * @see internal_commands
 * @see command
 */
command parse_input(std::string input) {
    boost::trim(input);
    if (input.empty()) {
        return {};
    }

    tokens_t tokens = lexer(input);
    check_syntax(tokens);

    return split_commands(tokens);
}
