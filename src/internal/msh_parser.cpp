//
// Created by andrew on 10/9/23.
//

#include "internal/msh_parser.h"
#include "internal/msh_utils.h"
#include "internal/msh_builtin.h"
#include "internal/msh_exec.h"

#include <boost/algorithm/string.hpp>

std::vector<Token> lexical_analysis(const std::string &input) {
    std::vector<Token> tokens;
    Token currentToken;
    bool is_quotes = false;
    size_t i = 0, len = input.length();

    while (i < len) {
        char current_char = input[i];

        if (current_char == '\'' || current_char == '\"') {
            is_quotes = !is_quotes;
        }

        if (currentToken.open_until) {
            if (current_char == currentToken.open_until)
                currentToken.open_until = '\0';
            else
                currentToken.value += current_char;
            ++i;
            continue;
        }

        switch (current_char) {
            case '\\':
                if (currentToken.type != WORD) {
                    tokens.push_back(currentToken);
                    currentToken = Token(WORD);
                }
                i++;
                currentToken.value += input[i];
                break;
            case '&':
                if (currentToken.type == AMP) currentToken.type = AND;
                else {
                    tokens.push_back(currentToken);
                    currentToken = Token(AMP, "&");
                }
                break;
            case '|':
                if (currentToken.type == PIPE) currentToken.type = OR;
                else {
                    tokens.push_back(currentToken);
                    currentToken = Token(PIPE, "|");
                }
                break;
            case '>':
                tokens.push_back(currentToken);
                currentToken = Token(OUT, '>');
                break;
            case '<':
                tokens.push_back(currentToken);
                currentToken = Token(IN, '<');
                break;
            case ';':
                tokens.push_back(currentToken);
                currentToken = Token(SEMICOLON, ';');
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
                if (!is_quotes && currentToken.type == WORD) {
                    currentToken.type = VAR_DECL;
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
                    currentToken = Token(COMMENT);
                    while (i < len && input[i] != '\n') {
                        i++;
                    }
                }
                break;
            case '(':
                tokens.push_back(currentToken);
                currentToken = Token(SUBOPEN, '(');
                break;
            case ')':
                tokens.push_back(currentToken);
                currentToken = Token(SUBCLOSE, ')');
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
    if (tokens[0].type == EMPTY) {
        tokens.erase(tokens.begin());
    }
    return tokens;
}

command parse_input(std::string input) {
    boost::trim(input);
    if (input.empty()) {
        return {};
    }

    std::vector<Token> tokens = lexical_analysis(input);


    std::vector<std::string> args;
    if (process_tokens(tokens, args) != 0) {
        return {};
    }

    std::string cmd = args[0];

    auto exec_func = is_builtin(cmd) ? internal_commands[cmd] : fork_exec;
    return {cmd, args, exec_func};
}
