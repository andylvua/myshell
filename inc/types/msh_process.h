//
// Created by andrew on 11/8/23.
//

#ifndef MYSHELL_MSH_PROCESS_H
#define MYSHELL_MSH_PROCESS_H

#include <map>
#include <sstream>
#include <string>
#include <vector>

using status_t = enum class status {
    RUNNING,
    STOPPED,
    DONE
};

/**
 * @brief Information about an internal process.
 *
 * @see msh_jobs
 */
struct process {
    status_t status = status::RUNNING;
    int flags = 0;
    std::string command;

    process(int flags, const std::vector<std::string>& args) : flags(flags) {
        std::stringstream ss;
        for (auto const& arg: args) {
            ss << arg << " ";
        }
        this->command = ss.str();
    }

    process() = default;

    /**
     * @brief Returns the status of the process as a string.
     *
     * @return The string representation of the process status.
     */
    [[nodiscard]] std::string get_status() const {
        switch (status) {
            using enum status_t;
            case RUNNING:
                return "Running";
            case STOPPED:
                return "Stopped";
            case DONE:
                return "Done";
            default:
                return "Unknown";
        }
    }
};


#endif //MYSHELL_MSH_PROCESS_H
