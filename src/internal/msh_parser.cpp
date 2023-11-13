// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/9/23.
//

#include "internal/msh_parser.h"
#include "internal/msh_utils.h"
#include "internal/msh_builtin.h"

#include <boost/algorithm/string.hpp>
#include <stack>

/**
 * Perform lexical analysis on the given input string, breaking it down into a vector of tokens.
 *
 * @param input The input string to be analyzed.
 * @return A vector of Token objects.
 */
tokens_t lexer(const std::string &input) {
    using enum TokenType;

    tokens_t tokens;
    Token current_token;
    bool command_expected = true;
    char current_char, next_char, open_until = '\0';
    size_t i = 0, len = input.length();
    std::stack<char> substitutions;

    while (i < len) {
        current_char = input[i];
        next_char = i + 1 < len ? input[i + 1] : '\0';

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
            } else if (current_char == '\\' && next_char == '"' && open_until == '"') {
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
                current_token.value += next_char;
                i++;
                break;
            case '&':
                if (current_token.type == AMP) current_token.set_type(AND);
                else {
                    tokens.push_back(current_token);
                    if (next_char == '>') {
                        current_token = Token(AMP_OUT, "&>");
                        ++i;
                    } else {
                        current_token = Token(AMP, "&");
                    }
                }
                break;
            case '|':
                if (current_token.type == PIPE) current_token.set_type(OR);
                else {
                    tokens.push_back(current_token);
                    current_token = Token(PIPE, "|");
                }
                break;
            case '>':
                if (current_token.type == OUT) current_token.set_type(OUT_APPEND);
                else {
                    tokens.push_back(current_token);
                    if (next_char == '&') {
                        current_token = Token(OUT_AMP, ">&");
                        ++i;
                    } else {
                        current_token = Token(OUT, ">");
                    }
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
                if (open_until == '\0' && command_expected) {
                    current_token.set_type(VAR_DECL);
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
