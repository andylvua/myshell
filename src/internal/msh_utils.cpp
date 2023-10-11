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


void print_error(const std::string &msg) {
    std::cerr << "myshell: " << msg << std::endl;
}

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

}

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
            if(stack.empty()) {
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

void expand_vars(std::vector<Token> &tokens) {
    std::string stop_chars = "$=:\'\" \t\n";

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

void squash_tokens(std::vector<Token> &tokens) {
    for (size_t i = 0; i < tokens.size() - 1; i++) {
        if (token_flags[tokens[i].type] & WORD_LIKE && token_flags[tokens[i + 1].type] & WORD_LIKE) {
            tokens[i].value += tokens[i + 1].value;
            tokens.erase(tokens.begin() + static_cast<int>(i) + 1);
            i--;
        }
    }
}

void split_tokens(std::vector<Token> &tokens, std::vector<std::string> &args) {
    for (auto &token: tokens) {
        if (token_flags[token.type] & WORD_LIKE) {
            args.push_back(token.value);
        }
    }
}

int check_syntax(std::vector<Token> &tokens) {
    for (auto &token: tokens) {
        if (token_flags[token.type] & UNSUPPORTED) {
            print_error("Unsupported token: " + token.value);
            return 1;
        }
    }

    return 0;
}

int process_tokens(std::vector<Token> &tokens, std::vector<std::string> &args) {
    expand_aliases(tokens);
    expand_vars(tokens);
    set_variables(tokens);
    squash_tokens(tokens);
    expand_glob(tokens);
    split_tokens(tokens, args);

    return check_syntax(tokens);
}
