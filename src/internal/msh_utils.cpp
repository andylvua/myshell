//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"
#include "types/msh_command.h"
#include "internal/msh_utils.h"
#include "internal/msh_exec.h"

#include <glob.h>
#include <vector>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

std::string prompt() {
    using boost::filesystem::current_path;
    std::string path = current_path().string();
    return path + " $ ";
}

void print_error(const std::string &msg) {
    std::cerr << "myshell: " << msg << std::endl;
}

void expand_glob(std::vector<std::string> &args) {
    std::vector<std::string> expanded_args;
    std::vector<std::string> args_copy = args;
    for (auto &arg: args_copy) {
        glob_t glob_result;
        glob(arg.data(), GLOB_TILDE, nullptr, &glob_result);
        if (glob_result.gl_pathc == 0) {
            globfree(&glob_result);
            continue;
        }
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            expanded_args.emplace_back(glob_result.gl_pathv[i]);
        }
        args.erase(std::remove(args.begin(), args.end(), arg), args.end());
        globfree(&glob_result);
    }
    args.insert(args.end(), expanded_args.begin(), expanded_args.end());
}

void expand_vars(std::vector<std::string> &args) {
    for (auto &arg: args) {
        std::string new_arg;
        for (size_t i = 0; i < arg.size(); i++) {
            if (arg[i] == '$') {
                std::string var_name;
                for (size_t j = i + 1; j < arg.size(); j++) {
                    if (arg[j] == '$' || arg[j] == ' ' || arg[j] == '\t') {
                        break;
                    }
                    var_name += arg[j];
                }
                if (var_name.empty()) {
                    continue;
                }
                if (getenv(var_name.data()) == nullptr) {
                    new_arg += '$';
                    continue;
                }
                new_arg += getenv(var_name.data());
                i += var_name.size();
            } else {
                new_arg += arg[i];
            }
        }
        arg = new_arg;
    }
}

void strip_comments(std::string &input) {
    input.erase(std::find(input.begin(), input.end(), '#'), input.end());
}

command parse_input(std::string input) {
    boost::trim(input);
    strip_comments(input);
    if (input.empty()) {
        return {"", {}, nullptr};
    }

    std::vector<std::string> args;
    boost::split(args, input, boost::is_any_of(" "), boost::token_compress_on);

    expand_vars(args);
    expand_glob(args);

    auto exec_func = is_internal(args[0]) ? internal_commands[args[0]] : fork_exec;
    return {args[0], args, exec_func};
}