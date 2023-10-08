//
// Created by andrew on 10/8/23.
//

#include "internal/msh_builtin.h"
#include "internal/msh_utils.h"

#include <fstream>

static char doc[] = "msource (.) -- Execute commands from a file in the current shell environment.";
static char args_doc[] = "<file>";

int msource(int argc, char **argv) {
    if (handle_help(argc, argv, doc, args_doc)) {
        return 0;
    }

    std::ifstream script(argv[1]);
    if (!script.good()) {
        std::cout << "Couldn't open file " << argv[1] << std::endl;
        return 1;
    }
    std::string line;
    while (std::getline(script, line)) {
        auto command = parse_input(line);
        command.execute();
    }
    return 0;
}
