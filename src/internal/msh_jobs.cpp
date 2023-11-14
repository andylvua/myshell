//
// Created by andrew on 11/8/23.
//

/**
 * Simplest job control implementation. Just for demonstration purposes.
 * Features only process tracking and printing.
 *
 * MAYBE: In future implement full job control supporting foreground and background processes
 *  with respecting process groups and signals handling process pipelines.
 */

#include "internal/msh_jobs.h"
#include "internal/msh_builtin.h"
#include "internal/msh_exec.h"

#include <algorithm>
#include <sys/wait.h>
#include <iostream>


std::map<pid_t, process> processes;


void init_job_control() {
    struct sigaction sa{};
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        msh_error("failed to set SIGCHLD handler" + std::string(strerror(errno)));
    }
}

void sigchld_handler(int) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            set_process_status(pid, status::DONE);
        } else if (WIFSTOPPED(status)) {
            set_process_status(pid, status::STOPPED);
        }
    }
}

int wait_for_process(pid_t pid, int *status) {
    while (waitpid(pid, status, WUNTRACED | WCONTINUED) != -1) {
        if (errno != EINTR && errno != ECHILD) {
            msh_error(strerror(errno));
            return -1;
        }
    }

    if (WIFEXITED(*status)) {
        *status = WEXITSTATUS(*status);
    } else if (WIFSIGNALED(*status)) {
        *status = WTERMSIG(*status);
    }

    remove_process(pid);
    return *status;
}

int reap_children() {
    int status;
    pid_t pid;
    int n = 0;
    while ((pid = waitpid(-1, &status, 0)) > 0) {
        remove_process(pid);
        n++;
    }
    return n;
}

int no_background_processes() {
    int n = 0;
    for (auto const& [pid, process]: processes) {
        if (process.status == status::RUNNING) {
            n++;
        }
    }
    return n;
}

void print_processes() {
    int n = 0;
    for (auto const& [pid, process]: processes) {
        std::cout << "[" << ++n << "] " << process.get_status() << "\t" << process.command << std::endl;
    }
}

void remove_completed_processes() {
    std::erase_if(processes, [](auto const& process) {
        return process.second.status == status::DONE;
    });
}

void print_completed_processes() {
    int n = 0;
    for (auto const& [pid, process]: processes) {
        if (process.status == status::DONE && process.flags & ASYNC) {
            std::cout << "[" << ++n << "] " << process.get_status() << "\t" << process.command << std::endl;
        }
    }
}

void update_jobs() {
    print_completed_processes();
    remove_completed_processes();
}

void add_process(pid_t pid, int flags, const std::vector<std::string>& args) {
    processes.try_emplace(pid, flags, args);
}

void remove_process(pid_t pid) {
    processes.erase(pid);
}

void set_process_status(pid_t pid, status_t status) {
    processes[pid].status = status;
}
