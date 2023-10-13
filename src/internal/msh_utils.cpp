// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#include "types/msh_command.h"
#include "types/msh_variable.h"
#include "types/msh_token.h"
#include "internal/msh_utils.h"
#include "internal/msh_internal.h"
#include "internal/msh_builtin.h"
#include "internal/msh_parser.h"

#include <glob.h>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>
#include <boost/asio.hpp>


/**
 * @brief Print an error message to stderr with a `myshell: ` prefix.
 *
 * @param msg Error message.
 */
void print_error(const std::string &msg) {
    std::cerr << "myshell: " << msg << std::endl;
}

/**
 * Sets internal variables based on VAR_DECL tokens in a vector of tokens.
 *
 * Iterates through a vector of tokens, identifies VAR_DECL tokens, and sets corresponding
 * variables based on their values. If a VAR_DECL token is followed by a token flagged as IS_STRING
 * without separators, the two tokens are combined and the resulting string is assigned to the variable.
 *
 * @param tokens A vector of tokens to process and update variable values.
 * @note VAR_DECL tokens are removed from the vector after processing.
 *
 * @see Token
 * @see token_flags
 * @see set_variable
 */
void set_variables(std::vector<Token> &tokens) {
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        auto &token = *it;
        if (token.type == VAR_DECL) {
            auto next_token = (it + 1);
            if (next_token != tokens.end() && token_flags[next_token->type] & IS_STRING) {
                token.value += next_token->value;
                tokens.erase(next_token);
            }
            auto pos = token.value.find('=');
            auto var_name = token.value.substr(0, pos);
            auto var_value = token.value.substr(pos + 1);
            set_variable(var_name, var_value);
        }
    }

    tokens.erase(std::remove_if(tokens.begin(), tokens.end(), [](const Token &token) {
        return token.type == VAR_DECL;
    }), tokens.end());

    // TODO! Handle illegal variable names. Only valid variable declarations should be processed
    //  and removed from the vector, otherwise they should be treated as WORD tokens.
}

/**
 * @brief Expand command aliases within a vector of tokens.
 *
 * Takes a vector of tokens and expands command aliases by replacing alias, defined with @c malias, with their
 * corresponding token sequences. Uses a stack-based approach to perform alias expansion, ensuring that
 * nested aliases are also expanded. Only COMMAND tokens are eligible for alias expansion.
 *
 * @param tokens A vector of tokens to expand aliases within.
 * @note Alias expansion is performed in-place, i.e. the input vector is modified.
 *
 * @see malias
 * @see Token
 * @see token_flags
 * @see aliases
 * @see lexical_analysis()
 */
void expand_aliases(std::vector<Token> &tokens) {
    std::vector<std::string> expanded;
    expanded.reserve(aliases.size());
    std::stack<std::pair<int, std::vector<Token>>> stack;
    stack.emplace(0, tokens);

    size_t expansion_pointer = 0;
    while (!stack.empty()) {
        auto &curr_tokens = stack.top().second;

        for (; expansion_pointer < curr_tokens.size(); expansion_pointer++) {
            auto &token = curr_tokens[expansion_pointer];

            if (token.type != COMMAND) {
                continue;
            }
            if (std::find(expanded.begin(), expanded.end(), token.value) != expanded.end()) {
                continue;
            }

            if (auto alias = aliases.find(token.value); alias != aliases.end()) {
                expanded.push_back(token.value);
                stack.emplace(expansion_pointer, lexical_analysis(alias->second));
                expansion_pointer = 0;
                break;
            }
        }

        if (expansion_pointer >= curr_tokens.size()) {
            auto [insert_to, top_tokens] = stack.top();

            stack.pop();
            if (stack.empty()) {
                tokens = top_tokens;
                continue;
            }

            auto &last_tokens = stack.top().second;

            expanded.pop_back();
            last_tokens.insert(last_tokens.begin() + insert_to + 1, top_tokens.begin(), top_tokens.end());
            last_tokens.erase(last_tokens.begin() + insert_to);

            expansion_pointer = insert_to + static_cast<int>(top_tokens.size());
        }
    }
}

/**
 * @brief Expand variables within a vector of tokens.
 *
 * Iterates through a vector of tokens and expands variables within tokens by
 * replacing them with their corresponding values. The expansion priority is given to internal variables.
 * If no variable with the given name is found, expansion result to an empty string.
 *
 * Tokens flagged as VAR_NO_EXPAND are not expanded.
 *
 * @param tokens A vector of tokens to process and expand variables within.
 * @note Variable expansion is performed in-place, i.e. the input vector is modified.
 *
 * @see Token
 * @see token_flags
 * @see get_variable
 */
void expand_vars(std::vector<Token> &tokens) {
    std::string stop_chars = "$=:\'\" \t\n";
    // TODO! Move to a global constant. Add variable declaration
    //  syntax to documentation.

    for (auto &token: tokens) {
        if (not(token_flags[token.type] & VAR_NO_EXPAND)) {
            std::string new_value;
            for (size_t i = 0; i < token.value.size(); i++) {
                if (token.value[i] == '$') {
                    std::string var_name;
                    for (size_t j = i + 1; j < token.value.size(); j++) {
                        if (stop_chars.find(token.value[j]) != std::string::npos) {
                            break;
                        }
                        var_name += token.value[j];
                    }
                    if (var_name.empty()) {
                        new_value += token.value[i];
                        continue;
                    }
                    auto internal_var = get_variable(var_name);
                    if (internal_var != nullptr) {
                        new_value += internal_var->value;
                        i += var_name.size();
                        continue;
                    }
                    auto var = getenv(var_name.data());
                    i += var_name.size();

                    if (var == nullptr) {
                        continue;
                    }
                    new_value += var;
                } else {
                    new_value += token.value[i];
                }
            }
            token.value = std::move(new_value);
        }
    }
}

/**
 * @brief Expand glob patterns within a vector of tokens.
 *
 * Iterates through a vector of tokens and expands glob patterns within tokens by
 * matching them against files in the file system. If no files are matched, the glob pattern is left
 * unchanged. Token with matched glob pattern is replaced with a sequence of WORD tokens
 * containing the matched file names.
 *
 * Tokens flagged as GLOB_NO_EXPAND are not expanded.
 *
 * @param tokens A vector of tokens to process and expand glob patterns within.
 * @note Glob expansion is performed in-place, i.e. the input vector is modified.
 *
 * @see Token
 * @see token_flags
 * @see glob
 */
void expand_glob(std::vector<Token> &tokens) {
    for (size_t i = 0; i < tokens.size(); i++) {
        if (not(token_flags[tokens[i].type] & GLOB_NO_EXPAND)) {
            std::vector<Token> expanded_tokens;

            glob_t glob_result;
            glob(tokens[i].value.data(), GLOB_TILDE, nullptr, &glob_result);
            if (glob_result.gl_pathc == 0) {
                globfree(&glob_result);
                continue;
            }
            for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                expanded_tokens.emplace_back(WORD, glob_result.gl_pathv[j]);
                if (j != glob_result.gl_pathc - 1) {
                    expanded_tokens.emplace_back(EMPTY);
                }
            }
            auto insert_to = tokens.begin() + static_cast<int>(i);
            tokens.erase(insert_to);
            tokens.insert(insert_to, expanded_tokens.begin(), expanded_tokens.end());
            i += static_cast<int>(expanded_tokens.size()) - 1;
            globfree(&glob_result);
        }
    }
}

/**
 * @brief Squash the adjacent tokens flagged as WORD_LIKE.
 * @param tokens A vector of tokens to process.
 */
void squash_tokens(std::vector<Token> &tokens) {
    for (size_t i = 0; i < tokens.size() - 1; i++) {
        if (token_flags[tokens[i].type] & WORD_LIKE && token_flags[tokens[i + 1].type] & WORD_LIKE) {
            tokens[i].value += tokens[i + 1].value;
            tokens.erase(tokens.begin() + static_cast<int>(i) + 1);
            i--;
        }
    }
}

/**
 * @brief Check the syntax of a vector of tokens.
 *
 * @param tokens A vector of tokens to check.
 * @return 0 if syntax is correct, 1 otherwise.
 *
 * @see Token
 * @see token_flags
 */
int check_syntax(std::vector<Token> &tokens) {
    for (auto &token: tokens) {
        if (token_flags[token.type] & UNSUPPORTED) {
            print_error("Unsupported token: " + token.value);
            return 1;
        }
    }

    return 0;

    // TODO! Currently only checks for unsupported tokens. Add proper syntax checking for
    //  unclosed delimiters, unexpected tokens, etc. This will also enable multiline input handling.
}


/**
 * @brief Split a vector of tokens into a vector of arguments for further execution.
 *
 * Only WORD_LIKE tokens are extracted.
 *
 * @param tokens A vector of tokens to split.
 * @return A vector of strings containing the extracted tokens.
 *
 * @see Token
 * @see token_flags
 */
std::vector<std::string> split_tokens(std::vector<Token> &tokens) {
    std::vector<std::string> args;
    for (auto &token: tokens) {
        if (token_flags[token.type] & WORD_LIKE) {
            args.push_back(token.value);
        }
    }
    return args;
}


/**
 * @brief Process a vector of tokens.
 *
 * Performs all necessary token processing steps in the correct order.
 *
 * @param tokens A vector of tokens to process.
 * @return 0 if syntax is correct, 1 otherwise.
 *
 * @see expand_aliases
 * @see expand_vars
 * @see set_variables
 * @see expand_glob
 * @see squash_tokens
 * @see check_syntax
 */
int process_tokens(std::vector<Token> &tokens) {
    expand_aliases(tokens);
    expand_vars(tokens);
    set_variables(tokens);
    expand_glob(tokens);
    squash_tokens(tokens);

    return check_syntax(tokens);
}
