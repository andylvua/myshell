//
// Created by andrew on 11/8/23.
//

#ifndef MYSHELL_MSH_JOBS_H
#define MYSHELL_MSH_JOBS_H

/**
 * Simplest job control implementation. Just for demonstration purposes.
 * Features only process tracking and printing.
 *
 * MAYBE: In future implement full job control supporting foreground and background processes
 *  with respecting process groups and signals handling process pipelines.
 */

#include "types/msh_process.h"

#include <map>
#include <sstream>

struct process;
extern std::map<pid_t, process> processes;

void init_job_control();

void sigchld_handler(int);

int wait_for_process(pid_t pid, int *status);

int reap_children();

int no_background_processes();

void print_processes();

void print_completed_processes();

void update_jobs();

void add_process(pid_t pid, int flags, const std::vector<std::string> &args);

void remove_process(pid_t pid);

void set_process_status(pid_t pid, status_t status);


#endif //MYSHELL_MSH_JOBS_H
