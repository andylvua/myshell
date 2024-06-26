// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "internal/msh_builtin.h"
#include "internal/msh_utils.h"
#include "internal/msh_parser.h"
#include "internal/msh_internal.h"

#include <cstdio>
#include <readline/readline.h>
#include <readline/history.h>

// MAYBE: Add signal handling. Also see src/internal/jobs.cpp.
//  Possible behavior: https://www.gnu.org/software/bash/manual/html_node/Signals.html
int main(int argc, char *argv[]) {
    msh_init();

    if (argc > 1) {
        msource(argc, argv);
        return 0;
    }

    rl_reset_terminal(nullptr); // To prevent `readline` from messing up the terminal.

    char *input_buffer;
    while ((input_buffer = readline(generate_prompt().data())) != nullptr) {
        if (input_buffer[0] != '\0') {
            add_history(input_buffer);
        }

        update_jobs();
        try {
            auto command = parse_input(input_buffer);
            command.execute();
        } catch (const msh_exception &e) {
            msh_error(e.what());
            msh_errno = e.code();
        }

        free(input_buffer);
        std::cout << std::endl;
    }

    return 0;
}
