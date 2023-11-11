// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/10/23.
//

#include "internal/msh_prompt.h"
#include "internal/msh_error.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/filesystem.hpp>
#include <readline/readline.h>

/**
 * @brief Expand a PS1 (Prompt String 1) format string into its corresponding values.
 *
 * This function takes a PS1 format string as input and processes escape sequences to replace them with
 * specific values. The supported escape sequences include:
 *
 * <li>\\d - The current date in YYYY-MM-DD format.</li>
 * <li>\\t - The current time in HH:MM:SS format.</li>
 * <li>\\u - The current user.</li>
 * <li>\\h - The current host.</li>
 * <li>\\w - The current working directory.</li>
 * <li>\\W - The current working directory's basename.</li>
 * <li>\\n - A newline character.</li>
 * <li>\\r - A carriage return character.</li>
 * <li>\\s - The current shell.</li>
 * <li>\\v - The current shell version.</li>
 * <li>\\$ - The prompt character.</li>
 *
 * @param ps1 The PS1 format string to be expanded.
 * @return The expanded string with replaced escape sequences.
 */
std::string expand_ps1(const std::string &ps1) {
    std::string result;

    for (size_t i = 0; i < ps1.size(); i++) {
        if (ps1[i] != '\\' || i + 1 >= ps1.size()) {
            result += ps1[i];
            continue;
        }

        auto next = ps1[i + 1];

        // Check for \u, \s, \v escape sequences, as they require special handling
        if (next == 'u') {
            auto user = getenv("USER");
            result += user != nullptr ? user : "";
            goto next;
        } else if (next == 's') {
            auto shell = getenv("SHELL");
            result += shell != nullptr ? shell : "";
            goto next;
        } else if (next == 'v') {
            auto version = getenv("VERSION");
            result += version != nullptr ? version : "";
            goto next;
        }

        switch (next) {
            case 'd':
                namespace bg = boost::gregorian;
                result += bg::to_simple_string(bg::day_clock::local_day());
                break;
            case 't':
                namespace pt = boost::posix_time;
                result += pt::to_simple_string(pt::second_clock::local_time()).substr(12);
                break;
            case 'h':
                result += boost::asio::ip::host_name();
                break;
            case 'w':
                result += boost::filesystem::current_path().string();
                break;
            case 'W':
                result += boost::filesystem::current_path().filename().string();
                break;
            case 'n':
                result += '\n';
                break;
            case 'r':
                result += '\r';
                break;
            case '$':
                result += '$';
                break;
            default:
                result += next;
                break;
        }

        next:
        i++;
    }
    return result;
}

/**
 * @brief Generate a prompt string.
 *
 * This function generates a prompt string by expanding the PS1 environment variable.
 * If the PS1 environment variable is not set, the default prompt defined in msh_prompt.h is used.
 *
 * @return The generated prompt string.
 */
std::string generate_prompt() {
    static auto constexpr BACKGROUND = "\033[48;5;236m";
    static auto constexpr FOREGROUND = "\033[38;5;236m";
    static auto constexpr RESET = "\033[0m";
    static auto constexpr MARKER_SUCCESS = "✔";
    static auto constexpr MARKER_FAILURE = "✘";

    int cols = 0;
    rl_get_screen_size(nullptr, &cols);

    std::string prompt;
    if (auto ps1 = getenv("PS1"); ps1 == nullptr) {
        prompt = expand_ps1(DEFAULT_PS1);
    } else {
        prompt = expand_ps1(ps1);
    }

    auto marker_color = msh_errno == 0 ? "\033[32m" : "\033[31m";
    auto exit_marker = BACKGROUND + std::string(" ") + marker_color;
    auto indicator = msh_errno == 0 ? MARKER_SUCCESS : MARKER_FAILURE;

    exit_marker += (msh_errno == 0 ? "" : std::to_string(msh_errno) + " ") + indicator + " " + RESET;
    auto indicator_pos = std::to_string(cols - exit_marker.size() + 36);

    std::string escape_seq;

    escape_seq += BACKGROUND + prompt + BACKGROUND + " \033[1;37m|";
    escape_seq += exit_marker;
    escape_seq += FOREGROUND + std::string("") + RESET + " ";

    return escape_seq;
}
