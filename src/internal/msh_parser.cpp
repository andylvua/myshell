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
    tokens_t tokens;
    Token currentToken, prev_token;
    bool is_quotes = false, command_expected = true;
    char current_char, next_char;
    size_t i = 0, len = input.length();

    while (i < len) {
        current_char = input[i];
        next_char = i + 1 < len ? input[i + 1] : '\0';
        prev_token = currentToken;

        if (!tokens.empty() && tokens.back().type == TokenType::WORD && command_expected) {
            tokens.back().set_type(TokenType::COMMAND);
            command_expected = false;
        }
        command_expected |= currentToken.get_flag(COMMAND_SEPARATOR);

        if (current_char == '\'' || current_char == '\"') {
            is_quotes = !is_quotes;
        }

        if (currentToken.open_until) {
            if (current_char == currentToken.open_until) {
                currentToken.open_until = '\0';
            } else if (current_char == '\\' && next_char == '"' && currentToken.open_until == '\"') {
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
                if (currentToken.type != TokenType::WORD) {
                    tokens.push_back(currentToken);
                    currentToken = Token(TokenType::WORD);
                }
                currentToken.value += next_char;
                i++;
                break;
            case '&':
                if (currentToken.type == TokenType::AMP) currentToken.set_type(TokenType::AND);
                else {
                    tokens.push_back(currentToken);
                    if (next_char == '>') {
                        currentToken = Token(TokenType::AMP_OUT, "&>");
                        ++i;
                    } else {
                        currentToken = Token(TokenType::AMP, "&");
                    }
                }
                break;
            case '|':
                if (currentToken.type == TokenType::PIPE) currentToken.set_type(TokenType::OR);
                else {
                    tokens.push_back(currentToken);
                    currentToken = Token(TokenType::PIPE, "|");
                }
                break;
            case '>':
                if (currentToken.type == TokenType::OUT) currentToken.set_type(TokenType::OUT_APPEND);
                else {
                    tokens.push_back(currentToken);
                    if (next_char == '&') {
                        currentToken = Token(TokenType::OUT_AMP, ">&");
                        ++i;
                    } else {
                        currentToken = Token(TokenType::OUT, ">");
                    }
                }
                break;
            case '<':
                tokens.push_back(currentToken);
                if (next_char == '&') {
                    currentToken = Token(TokenType::IN_AMP, "<&");
                    ++i;
                } else {
                    currentToken = Token(TokenType::IN, "<");
                }
                break;
            case ';':
                tokens.push_back(currentToken);
                currentToken = Token(TokenType::SEMICOLON, ";");
                break;
            case '\"':
                tokens.push_back(currentToken);
                currentToken = Token(TokenType::DQSTRING, '\"');
                break;
            case '\'':
                tokens.push_back(currentToken);
                currentToken = Token(TokenType::SQSTRING, '\'');
                break;
            case '=':
                if (!is_quotes && command_expected) {
                    currentToken.set_type(TokenType::VAR_DECL);
                }
                if (currentToken.type == TokenType::EMPTY) {
                    tokens.push_back(currentToken);
                    currentToken = Token(TokenType::WORD);
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
                currentToken = Token(TokenType::SUBOPEN, "(");
                break;
            case ')':
                tokens.push_back(currentToken);
                currentToken = Token(TokenType::SUBCLOSE, ")");
                break;
            case ' ':
                tokens.push_back(currentToken);
                currentToken = Token(TokenType::EMPTY);
                break;
            default:
                if (currentToken.type == TokenType::WORD || currentToken.type == TokenType::VAR_DECL) {
                    currentToken.value += current_char;
                } else {
                    tokens.push_back(currentToken);
                    currentToken = Token(TokenType::WORD);
                    currentToken.value = current_char;
                }
        }
        i++;
    }

    tokens.push_back(currentToken);
    tokens.erase(tokens.begin());
    if (!tokens.empty() && tokens.back().type == TokenType::WORD && command_expected) {
        tokens.back().set_type(TokenType::COMMAND);
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
