//
// Created by andrew on 11/8/23.
//

/**
 * @file
 * @brief Job control related utilities.
 *
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


/**
 * @brief The internal process table.
 *
 * Maps process IDs to process objects.
 */
std::map<pid_t, process> processes;


/**
 * @brief Initialize job control.
 *
 * Set SIGCHLD handler.
 *
 * @see sigchld_handler()
 */
void init_job_control() {
    struct sigaction sa{};
    sa.sa_handler = sigchld_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        msh_error("failed to set SIGCHLD handler" + std::string(strerror(errno)));
    }
}

/**
 * @brief SIGCHLD handler.
 *
 * Reaps the process and updates its internal status.
 *
 * @see waitpid()
 * @see set_process_status()
 *
 * @param signo The signal number.
 */
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

/**
 * @brief Wait for the process to finish.
 *
 * @param pid The process ID.
 * @param status The process status.
 * @return The process exit status.
 *
 * @note Call to this function leads to explicit removal of the process with matching PID
 * from the internal process table.
 *
 * @note If both `WIFEXITED` and `WIFSIGNALED` are false for the process, the
 * return value is undefined.
 *
 * @see remove_process()
 */
int wait_for_process(pid_t pid, int *status) {
    while (waitpid(pid, status, WUNTRACED | WCONTINUED) != -1) {}
    if (errno != ECHILD) {
        msh_error("failed to wait for process " + std::to_string(pid) + ": " + strerror(errno));
    }

    if (WIFEXITED(*status)) {
        *status = WEXITSTATUS(*status);
    } else if (WIFSIGNALED(*status)) {
        *status = WTERMSIG(*status);
    }

    remove_process(pid);
    return *status;
}

/**
 * @brief Wait for all background processes to finish.
 *
 * @return The number of reaped processes.
 *
 * @note All processes reaped by this function are immediately removed
 * from the internal process table.
 *
 * @see remove_process()
 */
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

/**
 * @brief Return the number of currently running background processes.
 *
 * @return The number of currently running background processes.
 */
int no_background_processes() {
    int n = 0;
    for (auto const& [pid, process]: processes) {
        if (process.status == status::RUNNING) {
            n++;
        }
    }
    return n;
}

/**
 * @brief Print all internal processes.
 */
void print_processes() {
    int n = 0;
    for (auto const& [pid, process]: processes) {
        std::cout << "[" << ++n << "] " << process.get_status() << "\t" << process.command << std::endl;
    }
}

/**
 * @brief Remove all completed processes, i.e. whose status is DONE
 * from the internal process table.
 */
void remove_completed_processes() {
    std::erase_if(processes, [](auto const& process) {
        return process.second.status == status::DONE;
    });
}

/**
 * @brief Print all completed background processes.
 *
 * @note Only prints processes that were started asynchronously.
 */
void print_completed_processes() {
    int n = 0;
    for (auto const& [pid, process]: processes) {
        if (process.status == status::DONE && process.flags & ASYNC) {
            std::cout << "[" << ++n << "] " << process.get_status() << "\t" << process.command << std::endl;
        }
    }
}

/**
 * @brief Update the internal process table.
 *
 * Is exactly equivalent to calling `print_completed_processes()`
 * and `remove_completed_processes()` sequentially.
 *
 * @note This function should be called before every command execution.
 *
 * @see print_completed_processes()
 * @see remove_completed_processes()
 */
void update_jobs() {
    print_completed_processes();
    remove_completed_processes();
}

/**
 * @brief Add a process to the internal process table.
 *
 * @param pid The process ID.
 * @param flags The process flags.
 * @param args The process arguments.
 *
 * @see process()
 */
void add_process(pid_t pid, int flags, const std::vector<std::string>& args) {
    processes.try_emplace(pid, flags, args);
}

/**
 * @brief Remove a process from the internal process table.
 *
 * @param pid The process ID.
 */
void remove_process(pid_t pid) {
    processes.erase(pid);
}

/**
 * @brief Change the status of a process in the internal process table.
 *
 * @param pid The process ID.
 * @param status The new process status.
 */
void set_process_status(pid_t pid, status_t status) {
    processes[pid].status = status;
}
