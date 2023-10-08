//
// Created by andrew on 10/9/23.
//

#include "msh_external.h"

#include <string>
#include <boost/algorithm/string.hpp>

bool is_msh_external(const char *cmd) {
    std::string external(EXTERNAL);
    std::string delimiter = ";";

    std::vector<std::string> programs;
    boost::split(programs, external, boost::is_any_of(delimiter));

    return std::find(programs.begin(), programs.end(), cmd) != programs.end();
}
