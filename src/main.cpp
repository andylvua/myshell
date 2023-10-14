// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "internal/msh_builtin.h"
#include "internal/msh_utils.h"
#include "internal/msh_parser.h"
#include "internal/msh_internal.h"

#include "msh_history.h"

#include <readline/readline.h>
#include <readline/history.h>

/**
 * @brief Write history to persistent file on exit.
 *
 * @note MSH_HISTORY_PATH is set automatically by the build system.
 *
 * @see MSH_HISTORY_PATH
 * @see write_history
 * @see atexit
 */
void on_exit() {
    write_history(MSH_HISTORY_PATH);
}

// MAYBE: Add signal handling
//  Possible behavior: https://www.gnu.org/software/bash/manual/html_node/Signals.html
int main(int argc, char *argv[]) {
    msh_init();
    atexit(on_exit);

    if (argc > 1) {
        msource(argc, argv);
        return 0;
    }

    char *input_buffer;
    read_history(MSH_HISTORY_PATH);

    while ((input_buffer = readline(generate_prompt().data())) != nullptr) {
        add_history(input_buffer);

        auto command = parse_input(input_buffer);
        command.execute();

        free(input_buffer);
    }

    return 0;
}
