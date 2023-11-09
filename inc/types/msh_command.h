// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//
// Created by andrew on 10/8/23.
//

#ifndef TEMPLATE_MSH_COMMAND_H
#define TEMPLATE_MSH_COMMAND_H

#include "internal/msh_error.h"
#include "internal/msh_builtin.h"
#include "internal/msh_exec.h"
#include "internal/msh_jobs.h"
#include "internal/msh_utils.h"
#include "internal/msh_redirects.h"

#include "msh_redirect.h"
#include "msh_token.h"
#include "msh_command_fwd.h"
#include "msh_exception.h"


#include <string>
#include <utility>
#include <vector>
#include <utility>
#include <iostream>
#include <algorithm>
#include <array>
#include <memory>
#include <variant>
#include <sys/wait.h>


struct command {
    std::variant<simple_command_ptr, connection_command_ptr> cmd;
    int flags = 0;

    int execute(int in = STDIN_FILENO, int out = STDOUT_FILENO) {
        std::visit([this, &in, &out](auto &&arg) {
            if (arg) {
                msh_errno = msh_exec_internal(*this, in, out, flags);
            }
        }, cmd);
        return msh_errno;
    }

    void set_flags(int flag) {
        flags |= flag;
    }
};


using simple_command_t = struct simple_command {
    using args_t = std::vector<std::string>;
    using argv_c_t = std::vector<char *>;
    using redirects_t = std::vector<redirect>;

    tokens_t tokens;
    args_t argv;
    argv_c_t argv_c;
    std::array<int, 3> saved_fds{};
    redirects_t redirects;
    int argc = 0;

    explicit simple_command(tokens_t tokens) : tokens(std::move(tokens)) {}

    bool construct() {
        for (auto const &token: tokens) {
            if (token.get_flag(WORD_LIKE) && !token.value.empty()) {
                argv.push_back(token.value);
            }
        }
        for (auto &arg: argv) {
            argv_c.push_back(arg.data());
        }
        argv_c.push_back(nullptr);
        return (argc = static_cast<int>(argv.size())) != 0;
    }

    int do_redirects(std::vector<int> *fd_to_close) {
        if (redirects.empty()) {
            return 0;
        }

        saved_fds[0] = dup(STDIN_FILENO);
        saved_fds[1] = dup(STDOUT_FILENO);
        saved_fds[2] = dup(STDERR_FILENO);
        for (auto const &redirect: redirects) {
            if (auto res = redirect.do_redirect(fd_to_close); res != 0) {
                return res;
            }
        }
        return 0;
    }

    void undo_redirects(std::vector<int> const &fd_to_close) {
        if (redirects.empty()) {
            return;
        }

        dup2(saved_fds[0], STDIN_FILENO);
        dup2(saved_fds[1], STDOUT_FILENO);
        dup2(saved_fds[2], STDERR_FILENO);
        std::ranges::for_each(saved_fds, close);
        std::ranges::for_each(fd_to_close, close);
    }

    int execute(int in = STDIN_FILENO, int out = STDOUT_FILENO, int flags = 0) {
        try {
            process_tokens(tokens);
            redirects = parse_redirects(tokens);
        } catch (msh_exception &e) {
            msh_error(e.what());
            return e.code();
        }

        if (!construct()) {
            return 0;
        }

        is_builtin(argv[0]) ? flags |= BUILTIN : flags |= 0;
        msh_errno = msh_exec_simple(*this, in, out, flags);

        return msh_errno;
    }
};

using connection_command_t = struct connection_command {
    Token connector;
    command lhs;
    command rhs;

    int execute(int in = STDIN_FILENO, int out = STDOUT_FILENO, int flags = 0) {
        switch (connector.type) {
            using enum TokenType;
            case PIPE:
                return execute_pipeline(in, out, flags);
            case SEMICOLON:
                return execute_sequence(flags);
            case AMP:
                return execute_background(flags);
            default:
                return UNKNOWN_ERROR; // FIXME: Add error handling
        }
    }

private:
    int execute_background(int flags) {
        lhs.set_flags(ASYNC);
        rhs.set_flags(flags & ASYNC);

        lhs.execute();
        rhs.execute();
        return msh_errno;
    }

    int execute_sequence(int flags) {
        rhs.set_flags(flags & ASYNC);

        lhs.execute();
        rhs.execute();
        return msh_errno;
    }

    int execute_pipeline(int in, int out, int flags) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            msh_error(strerror(errno));
            return UNKNOWN_ERROR; // FIXME: Add error handling
        }

        lhs.set_flags(flags | FORK_NO_WAIT);
        rhs.set_flags(flags);

        lhs.execute(in, pipefd[1]);
        close(pipefd[1]);
        rhs.execute(pipefd[0], out);
        close(pipefd[0]);

        if (!(flags & FORK_NO_WAIT) && !(flags & ASYNC)) {
            reap_children();
        }
        return msh_errno;
    }
};

#endif //TEMPLATE_MSH_COMMAND_H
