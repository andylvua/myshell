// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/10/23.
//

#include "internal/msh_prompt.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/filesystem.hpp>

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
        if (ps1[i] == '\\') {
            if (i + 1 < ps1.size()) {
                switch (ps1[i + 1]) {
                    case 'd':
                        namespace bg = boost::gregorian;
                        result += bg::to_simple_string(bg::day_clock::local_day());
                        break;
                    case 't':
                        namespace pt = boost::posix_time;
                        result += pt::to_simple_string(pt::second_clock::local_time()).substr(12);
                        break;
                    case 'u':
                        result += getenv("USER");
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
                    case 's':
                        result += getenv("SHELL");
                        break;
                    case 'v':
                        result += getenv("VERSION");
                        break;
                    case '$':
                        result += '$';
                        break;
                    default:
                        result += ps1[i + 1];
                        break;
                }
                i++;
            } else {
                result += ps1[i];
            }
        } else {
            result += ps1[i];
        }
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
   auto ps1 = getenv("PS1");
    if (ps1 == nullptr) {
        return expand_ps1(DEFAULT_PS1);
    }
    return expand_ps1(ps1);
}
