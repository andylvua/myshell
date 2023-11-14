// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//
/**
 * @file
 * @brief Tokens processing utilities.
 */

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
 * @brief Insert a sequence of tokens into a vector of tokens on a given position.
 *
 * @param tokens A vector of tokens to insert into.
 * @param pos An iterator pointing to the position to insert tokens at.
 * @param sub_tokens A vector of tokens to insert.
 *
 * @note The iterator is updated to point to the last inserted token. The token on the
 * given position is erased.
 *
 * @see Token
 * @see std::advance
 */
void insert_tokens(tokens_t &tokens, tokens_t::iterator &pos, const tokens_t &sub_tokens) {
    pos = tokens.erase(pos);
    pos = tokens.insert(pos, sub_tokens.begin(), sub_tokens.end());
    std::advance(pos, sub_tokens.size() - 1);
}

/**
 * @brief Perform word splitting on the given input string, breaking it down into a vector of WORD tokens.
 *
 * @param input The input string to be split.
 * @return A vector of WORD tokens.
 *
 * @note WORD tokens are separated according to the value of the IFS environment variable, if set,
 * otherwise the delimiters are <space>, <tab> and <newline>. Embedded newlines can be removed
 * during this operation.
 */
tokens_t split_words(const std::string_view input) {
    using namespace boost::algorithm;
    tokens_t tokens;

    const auto ifs = getenv("IFS");
    std::string delimeters = ifs != nullptr ? ifs : " \t\n";

    std::vector<std::string> words;
    split(words, input, is_any_of(delimeters), token_compress_on);

    for (auto &word: words) {
        tokens.emplace_back(TokenType::WORD, std::move(word));
        tokens.emplace_back(TokenType::EMPTY);
    }
    if (!tokens.empty()) {
        tokens.pop_back();
    }
    return tokens;
}

/**
 * @brief Sets internal variables based on VAR_DECL tokens in a vector of tokens.
 *
 * Iterates through a vector of tokens, identifies VAR_DECL tokens, and sets corresponding
 * variables based on their values. If a VAR_DECL token is followed by a token flagged as WORD_LIKE
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
            if (auto next = (it + 1); next != tokens.end() && next->get_flag(WORD_LIKE)) {
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
 * Tokens flagged as VAR_NO_EXPAND are not expanded. The word splitting is performed on the resulting
 * string, unless the token is flagged as NO_WORD_SPLIT.
 *
 * @param tokens A vector of tokens to process and expand variables within.
 * @note Variable expansion is performed in-place, i.e. the input vector is modified.
 *
 * @see Token
 * @see token_flags
 * @see get_variable
 */
void expand_vars(tokens_t &tokens) {
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        auto &token = *it;
        if (!token.get_flag(VAR_EXPAND)) {
            continue;
        }
        std::string new_value;
        for (size_t i = 0; i < token.value.size(); i++) {
            if (token.value[i] == '\\' && i + 1 < token.value.size() && token.value[i + 1] == '$') {
                new_value += token.value[++i];
                continue;
            }
            if (token.value[i] != '$') {
                new_value += token.value[i];
                continue;
            }
            std::string var_name;
            for (size_t j = i + 1; j < token.value.size(); j++) {
                if (!isalnum(token.value[j]) && token.value[j] != '_') {
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

        if (!token.get_flag(NO_WORD_SPLIT)) {
            auto sub_tokens = split_words(new_value);
            insert_tokens(tokens, it, sub_tokens);
        } else {
            token.value = std::move(new_value);
        }
    }
}

/**
 * @brief Perform command substitution within a vector of tokens.
 *
 * Iterates through a vector of tokens and replaces tokens of type COM_SUB with the output of the
 * underlying command by executing it in a subshell.
 *
 * Any trailing newlines are removed from the output. The word splitting is performed on the
 * resulting string, unless the token is flagged as NO_WORD_SPLIT.
 *
 * @param tokens A vector of tokens to process and substitute command substitutions within.
 * @note Command substitution is performed in-place, i.e. the input vector is modified.
 *
 * @throws msh_exception if an error occurs during command execution.
 *
 * @see Token
 * @see token_flags
 * @see parse_input
 */
void substitute_commands(tokens_t &tokens) {
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        auto &token = *it;
        if (token.type != TokenType::COM_SUB) {
            continue;
        }

        int pipefd[2];
        if (pipe(pipefd) == -1) {
            throw msh_exception("command substitution: " + std::string{strerror(errno)});
        }
        auto command = parse_input(token.value);
        command.set_flags(FORCE_PIPE);
        command.execute(STDIN_FILENO, pipefd[1]);
        close(pipefd[1]);

        std::string result;
        char buf[1024];
        ssize_t read_bytes;

        while (true) {
            read_bytes = read(pipefd[0], buf, sizeof(buf));
            if (read_bytes == -1) {
                if (errno == EINTR) {
                    continue;
                }
                throw msh_exception("command substitution: " + std::string{strerror(errno)});
            }
            if (read_bytes == 0) {
                break;
            }
            result.append(buf, read_bytes);
        }
        boost::trim_right_if(result, boost::is_any_of("\n"));

        if (!token.get_flag(NO_WORD_SPLIT)) {
            auto sub_tokens = split_words(result);
            insert_tokens(tokens, it, sub_tokens);
        } else {
            token.value = std::move(result);
        }
    }
}

/**
 * @brief Expand glob patterns within a vector of tokens.
 *
 * Iterates through a vector of tokens and expands glob patterns within tokens by
 * matching them against files in the file system. If no files are matched, the glob pattern
 * is left unchanged. Token with matched glob pattern is replaced with a sequence of
 * WORD tokens containing the matched file names.
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
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        auto &token = *it;
        if (!token.get_flag(GLOB_EXPAND)) {
            continue;
        }

        glob_t glob_result;
        glob(token.value.data(), GLOB_TILDE, nullptr, &glob_result);
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
        insert_tokens(tokens, it, expanded_tokens);
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
 * If syntax is incorrect, throws an exception.
 *
 * @param tokens A vector of tokens to check.
 *
 * @throws msh_exception if syntax is incorrect.
 *
 * @see Token
 * @see token_flags
 */
void check_syntax(const tokens_t &tokens) {
    for (auto const &token: tokens) {
        if (token.get_flag(UNSUPPORTED)) {
            throw msh_exception("unsupported token: " + std::string{token.value});
        }
    }

    // TODO! Currently only checks for unsupported tokens.
    //  Add proper syntax checking for unexpected tokens, etc. Add support for multiline input
}

/**
 * @brief Construct a shared pointer to a simple command from a vector of tokens.
 *
 * @param tokens A vector of tokens to construct a simple command from.
 * @return A shared pointer to a simple command.
 *
 * @see simple_command
 */
simple_command_ptr make_simple_command(const tokens_t &tokens) {
    simple_command command(tokens);

    return std::make_shared<simple_command>(std::move(command));
}

/**
 * @brief Split a vector of tokens into a tree structure of commands.
 *
 * Each node of the tree is a command object, which can be either a simple command or
 * a connection command. The connection command contains a connector token and two
 * command objects, lhs and rhs. Returns the root of the tree.
 *
 * @note Alias expansion is performed on whole command sequence before splitting.
 *
 * @param tokens A vector of tokens to split.
 * @return The root of the tree of commands.
 *
 * @see command
 * @see simple_command
 * @see connection_command
 */
command split_commands(tokens_t &tokens) {
    expand_aliases(tokens);

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
 *
 * @throws msh_exception if an error occurs during token processing.
 * Must be handled elsewhere.
 *
 * @see expand_vars
 * @see substitute_commands
 * @see set_variables
 * @see expand_glob
 * @see squash_tokens
 */
void process_tokens(tokens_t &tokens) {
    expand_vars(tokens);
    substitute_commands(tokens);
    set_variables(tokens);
    expand_glob(tokens);
    squash_tokens(tokens);
}
