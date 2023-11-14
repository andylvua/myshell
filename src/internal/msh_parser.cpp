// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/9/23.
//
/**
 * @file
 * @brief Parser related utilities.
 */

#include "internal/msh_parser.h"
#include "internal/msh_utils.h"
#include "internal/msh_builtin.h"

#include <boost/algorithm/string.hpp>
#include <stack>

/**
 * @brief Perform lexical analysis on the given input string, breaking it down into a vector of tokens.
 *
 * @param input The input string to be analyzed.
 * @return A vector of Token objects.
 *
 * @throws msh_exception If the input is invalid.
 *
 * @see parse_input()
 * @see process_tokens()
 */
tokens_t lexer(const std::string &input) {
    using enum TokenType;

    tokens_t tokens;
    Token current_token, previous_token;
    bool command_expected = true;
    char current_char, next_char, open_until = '\0';
    size_t i = 0, len = input.length();
    std::stack<char> substitutions;

    while (i < len) {
        current_char = input[i];
        next_char = i + 1 < len ? input[i + 1] : '\0';
        previous_token = current_token.type == EMPTY ? previous_token : current_token;

        if (!tokens.empty() && tokens.back().type == WORD && command_expected) {
            tokens.back().set_type(COMMAND);
            command_expected = false;
        }
        command_expected |= current_token.get_flag(COMMAND_SEPARATOR);

        if (current_char == open_until && substitutions.empty()) {
            open_until = '\0';
            ++i;
            continue;
        }

        if (open_until != '\0' && current_token.type == EMPTY) {
            switch (open_until) {
                case '"':
                    current_token.set_type(DQSTRING);
                    break;
                case '\'':
                    current_token.set_type(SQSTRING);
                    break;
                default:;
            }
        }

        if (open_until == '\'') {
            while (i < len && input[i] != '\'') {
                current_token.value += input[i];
                ++i;
            }
            continue;
        }

        if (current_char == '$' && next_char == '(') {
            if (!substitutions.empty()) {
                substitutions.push('\0');
                current_token.value += current_char;
                ++i;
                continue;
            }
            substitutions.push('\0');
            tokens.push_back(current_token);
            current_token = Token(COM_SUB);
            if (open_until == '"') {
                current_token.set_flag(NO_WORD_SPLIT);
            }
            i += 2;
            continue;
        }

        if (!substitutions.empty()) {
            if (current_char == '"' || current_char == '\'') {
                auto &top = substitutions.top();
                top = top == '\0' ? current_char : '\0';
            }
            if (current_char == ')' && substitutions.top() == '\0') {
                substitutions.pop();
                if (substitutions.empty()) {
                    tokens.push_back(current_token);
                    current_token = Token(EMPTY);
                    i++;
                    continue;
                }
            }
            current_token.value += current_char;
            ++i;
            continue;
        }

        if (open_until == '"') {
            if (current_char == '\\' && next_char == '\\') {
                current_token.value += current_char;
                ++i;
            } else if (current_char == '\\' && next_char == '"') {
                current_token.value += next_char;
                ++i;
            } else {
                current_token.value += current_char;
            }
            ++i;
            continue;
        }

        switch (current_char) {
            case '\\':
                if (current_token.type != WORD) {
                    tokens.push_back(current_token);
                    current_token = Token(WORD);
                }
                if (next_char == '$') {
                    current_token.value += current_char;
                } else {
                    current_token.value += next_char;
                    ++i;
                }
                break;
            case '&':
                tokens.push_back(current_token);

                switch (next_char) {
                    case '&':
                        current_token = Token(AND, "&&");
                        ++i;
                        break;
                    case '>':
                        current_token = Token(AMP_OUT, "&>");
                        ++i;
                        break;
                    default:
                        current_token = Token(AMP, "&");

                }
                break;
            case '|':
                tokens.push_back(current_token);
                switch (next_char) {
                    case '|':
                        current_token = Token(OR, "||");
                        ++i;
                        break;
                    case '&':
                        current_token = Token(PIPE_AMP, "|&");
                        ++i;
                        break;
                    default:
                        current_token = Token(PIPE, "|");
                }
                break;
            case '>':
                if (current_token.type == AMP_OUT) {
                    current_token.set_type(AMP_APPEND);
                    break;
                }

                tokens.push_back(current_token);
                switch (next_char) {
                    case '&':
                        current_token = Token(OUT_AMP, ">&");
                        ++i;
                        break;
                    case '>':
                        current_token = Token(OUT_APPEND, ">>");
                        ++i;
                        break;
                    default:
                        current_token = Token(OUT, ">");
                }
                break;
            case '<':
                tokens.push_back(current_token);
                if (next_char == '&') {
                    current_token = Token(IN_AMP, "<&");
                    ++i;
                } else {
                    current_token = Token(IN, "<");
                }
                break;
            case ';':
                tokens.push_back(current_token);
                current_token = Token(SEMICOLON, ";");
                break;
            case '\"':
                tokens.push_back(current_token);
                current_token = Token(DQSTRING);
                open_until = '\"';
                break;
            case '\'':
                tokens.push_back(current_token);
                current_token = Token(SQSTRING);
                open_until = '\'';
                break;
            case '=':
                if (open_until == '\0') {
                    if (command_expected) {
                        current_token.set_type(VAR_DECL);
                    } else {
                        current_token.set_flag(ASSIGNMENT_WORD);
                    }
                }
                if (current_token.type == EMPTY) {
                    tokens.push_back(current_token);
                    current_token = Token(WORD);
                }
                current_token.value += current_char;
                break;
            case '#':
                if (open_until == '\0') {
                    tokens.push_back(current_token);
                    while (i < len && input[i] != '\n') {
                        i++;
                    }
                }
                break;
            case '(':
                tokens.push_back(current_token);
                current_token = Token(SUBOPEN, "(");
                break;
            case ')':
                tokens.push_back(current_token);
                current_token = Token(SUBCLOSE, ")");
                break;
            case ' ':
                if (current_token.type != EMPTY) {
                    tokens.push_back(current_token);
                    current_token = Token(EMPTY);
                }
                break;
            default:
                if (current_token.type == WORD || current_token.type == VAR_DECL) {
                    current_token.value += current_char;
                } else {
                    tokens.push_back(current_token);
                    current_token = Token(WORD);
                    current_token.value = current_char;
                }
        }

        if (current_token.get_flag(COMMAND_SEPARATOR) && previous_token.get_flag(COMMAND_SEPARATOR)) {
            throw msh_exception("unexpected token: " + current_token.value, INTERNAL_ERROR);
        }
        i++;
    }

    if (current_token.type != EMPTY) {
        tokens.push_back(current_token);
    }

    if (!substitutions.empty()) {
        auto top = substitutions.top();
        if (top == '\0') {
            throw msh_exception("expected ')'", INTERNAL_ERROR);
        } else {
            throw msh_exception("expected '" + std::string(1, top) + "'", INTERNAL_ERROR);
        }
    }
    if (open_until != '\0') {
        throw msh_exception("unclosed delimiter: " + std::string(1, open_until), INTERNAL_ERROR);
    }

    tokens.erase(tokens.begin());
    if (!tokens.empty() && tokens.back().type == WORD && command_expected) {
        tokens.back().set_type(COMMAND);
    }

    return tokens;
}

/**
 * @brief Parse the input string to prepare it for command execution.
 *
 * Performs lexical analysis to tokenize the input, and checks for validity.
 * If the input is valid, it prepares a @c command object using @c split_commands() function.
 *
 * On empty input, the default-constructed @c command object is returned.
 *
 * @param input The input string to be parsed.
 * @return A @c command object.
 *
 * @throws msh_exception If the syntax of the input is invalid or errors occur during parsing.
 *
 * @see lexer()
 * @see command
 * @see split_commands()
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
