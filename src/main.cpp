// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "internal/msh_builtin.h"
#include "internal/msh_utils.h"
#include "internal/msh_parser.h"

#include "msh_history.h"

#include <readline/readline.h>
#include <readline/history.h>

void on_exit() {
    write_history(MSH_HISTORY_PATH);
}

int main(int argc, char *argv[]) {
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
    }

    return 0;
}
