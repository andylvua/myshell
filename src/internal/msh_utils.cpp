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

#include <glob.h>
#include <vector>
#include <algorithm>
#include <boost/filesystem.hpp>

std::string generate_prompt() {
    using boost::filesystem::current_path;
    std::string path = current_path().string();
    return path + " $ ";
}

void print_error(const std::string &msg) {
    std::cerr << "myshell: " << msg << std::endl;
}

void set_variables(std::vector<Token> &tokens) {
    for (auto &token: tokens) {
        if (token.type == VAR_DECL) {
            auto pos = token.value.find('=');
            auto var_name = token.value.substr(0, pos);
            auto var_value = token.value.substr(pos + 1);
            set_variable(var_name, var_value);
        }
    }
}

void expand_vars(std::vector<Token> &tokens) {
    std::string stop_chars = "$\'\" \t";

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
                        continue;
                    }
                    auto internal_var = get_variable(var_name);
                    if (internal_var != nullptr) {
                        new_value += internal_var->value;
                        i += var_name.size();
                        continue;
                    }
                    auto var = getenv(var_name.data());
                    if (var == nullptr) {
                        new_value += '$';
                        continue;
                    }
                    new_value += var;
                    i += var_name.size();
                } else {
                    new_value += token.value[i];
                }
            }
            token.value = std::move(new_value);
        }
    }
}

void expand_glob(std::vector<Token> &tokens) {
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        if (not(token_flags[it->type] & GLOB_NO_EXPAND)) {
            std::vector<Token> expanded_tokens;

            glob_t glob_result;
            glob(it->value.data(), GLOB_TILDE, nullptr, &glob_result);
            if (glob_result.gl_pathc == 0) {
                globfree(&glob_result);
                continue;
            }
            for (size_t j = 0; j < glob_result.gl_pathc; j++) {
                expanded_tokens.emplace_back(WORD, glob_result.gl_pathv[j]);
            }
            it = tokens.erase(it);
            tokens.insert(it, expanded_tokens.begin(), expanded_tokens.end());
            globfree(&glob_result);
        }
    }
}

int process_tokens(std::vector<Token> &tokens, std::vector<std::string> &args) {
    set_variables(tokens);
    expand_vars(tokens);
    expand_glob(tokens);

    tokens.erase(std::remove_if(tokens.begin(), tokens.end(), [](const Token &token) {
        return token.type == VAR_DECL;
    }), tokens.end());

    for (size_t i = 0; i < tokens.size(); i++) {
        if (token_flags[tokens[i].type] & UNSUPPORTED) {
            print_error("Unsupported token: " + tokens[i].value);
            return 1;
        }
        if (token_flags[tokens[i].type] & WORD_LIKE) {
            std::string arg = tokens[i].value;
            while (i + 1 < tokens.size() && token_flags[tokens[i + 1].type] & WORD_LIKE) {
                arg += tokens[i + 1].value;
                i++;
            }
            args.push_back(arg);
        }
    }
    return 0;
}

