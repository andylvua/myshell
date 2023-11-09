//
// Created by andrew on 11/8/23.
//

#include "internal/msh_builtin.h"
#include "internal/msh_jobs.h"

static const builtin_doc doc = {
        .name   = "mjobs",
        .args   = "[-h|--help]",
        .brief  = "Display information about jobs",
};

int mjobs(int argc, char **argv) {
    try {
        if (handle_help(argc, argv, doc)) {
            return 0;
        }
    } catch (const std::exception &e) {
        msh_error(doc.name + ": " + e.what());
        std::cerr << "Usage: " << doc.name << " " << doc.args << std::endl;
        return 1;
    }

    print_processes();

    return 0;
}
