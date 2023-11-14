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


/**
 * @brief Top-level command structure.
 *
 * Holds @c std::variant of @c simple_command_ptr and @c connection_command_ptr.
 *
 * On @c execute() the execution is delegated to the appropriate command using
 * @c msh_exec_internal().
 *
 * @see simple_command_t
 * @see connection_command_t
 * @see msh_exec_internal()
 */
struct command {
    std::variant<simple_command_ptr, connection_command_ptr> cmd;
    int flags = 0;

    /**
     * @brief Execute the command.
     *
     * @param in File descriptor to use as stdin.
     * @param out File descriptor to use as stdout
     * @return Exit code of the command.
     *
     * @see msh_exec_internal()
     */
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


/**
 * @brief Simple command structure.
 *
 * The minimal unit of execution. Requires a @c std::vector of @c tokens_t
 * to be constructed.
 *
 * On @c execute() the tokens are processed and the command is executed using
 * @c msh_exec_simple().
 *
 * @see msh_exec_simple()
 */
typedef struct simple_command {
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

    /**
     * @brief Construct the command.
     *
     * Constructs the command arguments array @c argv_c and the number of arguments @c argc.
     *
     * @note Should only be called after tokens processing.
     */
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

    /**
     * @brief Perform redirections attached to the command.
     *
     * @param fd_to_close Vector of file descriptors to close after the command is executed.
     * @return 0 on success, error code otherwise.
     *
     * @note All standard file descriptors are saved before performing the redirections.
     * @note File descriptors opened by redirections are added to @c fd_to_close.
     * They should be closed after the command is executed or earlier if the command fails.
     *
     * @see msh_redirects.h
     */
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

    /**
     * @brief Undo redirections attached to the command.
     *
     * @param fd_to_close Vector of file descriptors to close.
     *
     * @note Should be called after the command is executed or earlier if the command fails.
     *
     * Restores standard file descriptors and closes all file descriptors specified
     * by @c fd_to_close.
     */
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

    /**
     * @brief Execute the command.
     *
     * @param in File descriptor to use as stdin.
     * @param out File descriptor to use as stdout
     * @param flags Flags to pass to @c msh_exec_simple().
     * @return Exit code of the command.
     *
     * @see msh_exec_simple()
     */
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
} simple_command_t;


/**
 * @brief Connection command structure.
 *
 * Represents a connection between two commands. Holds a @c Token specifying the
 * type of connection and two @c command structures.
 *
 * On @c execute() the execution is delegated to the appropriate function
 * depending on the type of connection.
 *
 * @see command
 * @see Token
 */
typedef struct connection_command {
    Token connector;
    command lhs;
    command rhs;

    /**
     * @brief Execute the command.
     *
     * @param in File descriptor to use as stdin.
     * @param out File descriptor to use as stdout
     * @param flags Flags to pass to the command.
     * @return Exit code of the command.
     *
     * Executes the left-hand side command and then the right-hand side command
     * with respect to the type of connection.
     */
    int execute(int in = STDIN_FILENO, int out = STDOUT_FILENO, int flags = 0) {
        switch (connector.type) {
            using enum TokenType;
            case TokenType::PIPE_AMP:
                flags |= PIPE_STDERR;
                [[fallthrough]];
            case PIPE:
                return execute_pipeline(in, out, flags);
            case SEMICOLON:
                return execute_sequence(in, out, flags);
            case AMP:
                return execute_background(in, out, flags);
            case TokenType::AND:
            case TokenType::OR:
                return execute_conditional(in, out, flags, connector.type);
            default:
                return UNKNOWN_ERROR; // FIXME: Add error handling
        }
    }

private:
    /**
     * @brief Executes conditional connections, i.e. @c && and @c ||.
     *
     * If the left-hand side command returns 0 and the connection is @c && or
     * if the left-hand side command returns non-zero and the connection is @c ||,
     * the right-hand side command is executed.
     *
     * @param in File descriptor to use as stdin.
     * @param out File descriptor to use as stdout
     * @param flags Flags to pass to the command.
     * @param op Type of the connection.
     *
     * @return Exit code of the command.
     */
    int execute_conditional(int in, int out, int flags, TokenType op) {
        lhs.set_flags(flags & ASYNC);
        rhs.set_flags(flags & ASYNC);

        auto res = flags & FORCE_PIPE ? lhs.execute(in, out) : lhs.execute();
        if ((res == 0 && op == TokenType::AND) || (res != 0 && op == TokenType::OR)) {
            rhs.execute(in, out);
        }

        return res;
    }

    /**
     * @brief Executes background connections, i.e. @c &.
     *
     * Executes the left-hand side command and then the right-hand side command
     * asynchronously.
     *
     * @param in File descriptor to use as stdin.
     * @param out File descriptor to use as stdout
     * @param flags Flags to pass to the command.
     *
     * @return Exit code of the command.
     */
    int execute_background(int in, int out, int flags) {
        lhs.set_flags(ASYNC);
        rhs.set_flags(flags & ASYNC);

        flags & FORCE_PIPE ? lhs.execute(in, out) : lhs.execute();
        rhs.execute(in, out);
        return msh_errno;
    }

    /**
     * @brief Executes sequential connections, i.e. `;`.
     *
     * Executes the left-hand side command and then the right-hand side command
     * sequentially.
     *
     * @param in File descriptor to use as stdin.
     * @param out File descriptor to use as stdout
     * @param flags Flags to pass to the command.
     *
     * @return Exit code of the command.
     */
    int execute_sequence(int in, int out, int flags) {
        rhs.set_flags(flags & ASYNC);

        flags & FORCE_PIPE ? lhs.execute(in, out) : lhs.execute();
        rhs.execute(in, out);
        return msh_errno;
    }

    /**
     * @brief Executes pipeline connections, i.e. `|` and `|&`.
     *
     * Executes the left-hand side command and then the right-hand side command
     * sequentially with the output of the left-hand side command piped to the
     * input of the right-hand side command. If the connection is `|&`, the
     * standard error of the left-hand side command is also piped.
     *
     * @param in File descriptor to use as stdin.
     * @param out File descriptor to use as stdout
     * @param flags Flags to pass to the command.
     *
     * @return Exit code of the command.
     */
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
} connection_command_t;

#endif //TEMPLATE_MSH_COMMAND_H
