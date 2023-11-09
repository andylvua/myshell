//
// Created by andrew on 10/10/23.
//

#ifndef MYSHELL_MSH_PROMPT_H
#define MYSHELL_MSH_PROMPT_H

#include <string>

/* SUPPORTED VARIABLES */
// \d - the date in "YYYY-Mon-DD" format
// \t - the current time in "HH:MM:SS" format
// \u - the username of the current user
// \h - the hostname of the system
// \w - the current working directory
// \W - the basename of the current working directory
// \n - newline
// \r - carriage return
// \s - the name of the shell
// \v - the version of the shell
// \$ - a literal '$' prompt
/* END OF SUPPORTED VARIABLES */

constexpr auto DEFAULT_PS1 = "\033[1;38;5;250m \\u \033[1;37m| \033[1;94m\\W\033[0m";

std::string expand_ps1(const std::string &ps1);

std::string generate_prompt();

#endif //MYSHELL_MSH_PROMPT_H
