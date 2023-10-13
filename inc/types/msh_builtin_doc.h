//
// Created by andrew on 10/13/23.
//

#ifndef MYSHELL_MSH_BUILTIN_DOC_H
#define MYSHELL_MSH_BUILTIN_DOC_H

#include <string>

struct builtin_doc {
    std::string args;
    std::string brief;
    std::string doc{};
};

#endif //MYSHELL_MSH_BUILTIN_DOC_H
