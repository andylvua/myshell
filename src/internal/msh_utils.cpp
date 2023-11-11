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
#include "internal/msh_exec.h"

#include <glob.h>
#include <vector>
#include <list>
#include <stack>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/date_time.hpp>


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
void set_variables(tokens_t &tokens) {
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        auto &token = *it;
        if (token.type == TokenType::VAR_DECL) {
            if (auto next = (it + 1); next != tokens.end() && next->get_flag(IS_STRING)) {
                token.value += next->value;
                tokens.erase(next);
            }
            auto pos = token.value.find('=');
            auto var_name = token.value.substr(0, pos);
            auto var_value = token.value.substr(pos + 1);
            set_variable(var_name, var_value);
        }
    }

    // TODO! Handle illegal variable names. Only valid variable declarations should be processed
    //  and removed from the vector, otherwise they should be treated as WORD tokens and left unchanged
    // MAYBE: Handle temporary variable definitions - use it only for a simple command following the
    //  definition, then dispose of it
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
 * @see lexer()
 */
void expand_aliases(tokens_t &tokens) {
    std::vector<std::string> expanded;
    expanded.reserve(aliases.size());
    std::stack<std::pair<int, tokens_t>> stack;
    stack.emplace(0, tokens);

    size_t expansion_pointer = 0;
    while (!stack.empty()) {
        auto &curr_tokens = stack.top().second;

        for (; expansion_pointer < curr_tokens.size(); expansion_pointer++) {
            auto const &token = curr_tokens[expansion_pointer];

            if (token.type != TokenType::COMMAND) {
                continue;
            }
            if (std::ranges::find(expanded.begin(), expanded.end(), token.value) != expanded.end()) {
                continue;
            }

            if (auto alias = aliases.find(token.value); alias != aliases.end()) {
                expanded.push_back(token.value);
                stack.emplace(expansion_pointer, lexer(alias->second));
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
void expand_vars(tokens_t &tokens) {
    std::string stop_chars = "$=:\'\" \t\n";
    // TODO! Move to a global constant. Provide some documentation on
    //  metacharacters and syntax used by the shell

    for (auto &token: tokens) {
        if (!token.get_flag(VAR_EXPAND)) {
            continue;
        }
        std::string new_value;
        for (size_t i = 0; i < token.value.size(); i++) {
            if (token.value[i] != '$') {
                new_value += token.value[i];
                continue;
            }
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
            if (auto internal_var = get_variable(var_name); internal_var != nullptr) {
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
        }
        token.value = std::move(new_value);
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
void expand_glob(tokens_t &tokens) {
    tokens_t expanded_tokens;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (!tokens[i].get_flag(GLOB_EXPAND)) {
            continue;
        }
        glob_t glob_result;
        glob(tokens[i].value.data(), GLOB_TILDE, nullptr, &glob_result);
        if (glob_result.gl_pathc == 0) {
            globfree(&glob_result);
            continue;
        }
        expanded_tokens.reserve(glob_result.gl_pathc * 2 - 1);
        for (size_t j = 0; j < glob_result.gl_pathc; j++) {
            expanded_tokens.emplace_back(TokenType::WORD, glob_result.gl_pathv[j]);
            if (j != glob_result.gl_pathc - 1) {
                expanded_tokens.emplace_back(TokenType::EMPTY);
            }
        }
        auto insert_to = tokens.begin() + static_cast<int>(i);
        tokens.erase(insert_to);
        tokens.insert(insert_to, expanded_tokens.begin(), expanded_tokens.end());
        i += static_cast<int>(expanded_tokens.size()) - 1;
        globfree(&glob_result);
        expanded_tokens.clear();
    }
}

/**
 * @brief Squash the adjacent tokens flagged as WORD_LIKE.
 * @param tokens A vector of tokens to process.
 */
void squash_tokens(tokens_t &tokens) {
    if (tokens.empty()) {
        return;
    }
    std::transform(tokens.begin(), tokens.end() - 1, tokens.begin() + 1, tokens.begin(),
                   [](Token &a, Token &b) {
                       if (a.get_flag(WORD_LIKE) && b.get_flag(WORD_LIKE)) {
                           b.value = a.value + b.value;
                           a.set_type(TokenType::EMPTY);
                       }
                       return a;
                   });
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
void check_syntax(const tokens_t &tokens) {
    for (auto const &token: tokens) {
        if (token.get_flag(UNSUPPORTED)) {
            throw msh_exception("unsupported token: " + std::string{token.value});
        }
        if (token.open_until) {
            throw msh_exception("unclosed delimiter: " + std::string{token.open_until});
        }
    }

    // TODO! Currently only checks for unsupported tokens and unclosed delimiters.
    //  Add proper syntax checking for unexpected tokens, etc. Add support for multiline input
}

/**
 FIXME: Invalidated documentation
 */
simple_command_ptr make_simple_command(const tokens_t &tokens) {
    simple_command command(tokens);

    return std::make_shared<simple_command>(std::move(command));
}

/**
 FIXME: Invalidated documentation
 */
command split_commands(const tokens_t &tokens) {
	tokens_t current_command_tokens;
    struct command res_command;
    connection_command_ptr connection;
    auto simple = &res_command.cmd;

	for (const auto & token : tokens) {
	      if (token.get_flag(COMMAND_SEPARATOR)) {
              *simple = make_simple_command(current_command_tokens);

	          command new_cmd(std::make_shared<connection_command>());
              connection = std::get<connection_command_ptr>(new_cmd.cmd);
	          connection->lhs = res_command;
	          connection->connector = token;

              simple = &connection->rhs.cmd;

              res_command = new_cmd;
	          current_command_tokens.clear();
	          continue;
	      }
          current_command_tokens.push_back(token);
	}

    *simple = make_simple_command(current_command_tokens);
	return res_command;
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
void process_tokens(tokens_t &tokens) {
    expand_aliases(tokens);

    expand_vars(tokens);
    set_variables(tokens);

    expand_glob(tokens);
    squash_tokens(tokens);
}
