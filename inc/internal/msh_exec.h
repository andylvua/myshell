//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_MSH_EXEC_H
#define TEMPLATE_MSH_EXEC_H

extern int exec_line_no;
extern std::string exec_path;

int msh_exec_script(const char *path);

int msh_execve(char **argv);

int msh_exec(int argc, char **argv);

#endif //TEMPLATE_MSH_EXEC_H
