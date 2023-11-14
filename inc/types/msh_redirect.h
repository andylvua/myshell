//
// Created by andrew on 11/2/23.
//

#ifndef MYSHELL_MSH_REDIRECT_H
#define MYSHELL_MSH_REDIRECT_H

#include "internal/msh_error.h"
#include "msh_token.h"

#include <string>
#include <vector>
#include <utility>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>


struct redirect;
using redirects_t = std::vector<redirect>;

/**
 * @brief Structure representing a single redirectee.
 *
 * Holds either a file descriptor or a path to a file.
 *
 * @see redirect
 */
struct redirectee {
    int fd = -1;
    std::string path;

    /**
     * @brief Opens the redirection target.
     *
     * If the redirectee is a file descriptor, it is returned.
     * If the redirectee is a path, it is opened and the file descriptor
     * pointing to it is returned and added to @p fd_to_close.
     *
     * @param fd_to_close A vector of file descriptors to close.
     * @param flags Flags to pass to open(2).
     * @param mode Mode to pass to open(2).
     *
     * @return The file descriptor of the redirectee or -1 on error.
     */
    [[nodiscard]]
    int open_redirect(std::vector<int> *fd_to_close, int flags, int mode) const {
        int res = -1;
        if (this->fd != -1) {
            res = this->fd;
        } else if (!path.empty()) {
            res = open(path.c_str(), flags, mode);
            if (res == -1) {
                msh_error("cannot open " + path + ": " + strerror(errno));
                return -1;
            }
            if (fd_to_close != nullptr) {
                fd_to_close->push_back(res);
            }
        }
        return res;
    }
};

/**
 * @brief Structure representing a single redirection.
 *
 * Holds two redirectees and the type of the redirection.
 *
 * @see redirects_t
 */
struct redirect {
    struct redirectee in;
    struct redirectee out;
    bool both_err_out = false;

    enum {
        NONE,
        OUT,
        OUT_APPEND,
        IN,
    } type = NONE;

    explicit redirect(const Token& redirect_token) {
        switch (redirect_token.type) {
            case TokenType::IN:
            case TokenType::IN_AMP:
                type = IN;
                in.fd = STDIN_FILENO;
                break;
            case TokenType::AMP_OUT:
                both_err_out = true;
                [[fallthrough]];
            case TokenType::OUT_AMP:
            case TokenType::OUT:
                type = OUT;
                in.fd = STDOUT_FILENO;
                break;
            case TokenType::AMP_APPEND:
                both_err_out = true;
                [[fallthrough]];
            case TokenType::OUT_APPEND:
                type = OUT_APPEND;
                in.fd = STDOUT_FILENO;
                break;
            default:
                break;
        }
    }

    /**
     * @brief Opens the redirectees with respect to the redirection type
     * and duplicates the appropriate file descriptors.
     *
     * The @p fd_to_close vector is populated with any file descriptors
     * that were opened during the redirection process.
     *
     * @param fd_to_close A vector of file descriptors to close.
     *
     * @return 0 on success, 1 on error.
     */
    [[nodiscard]] int do_redirect(std::vector<int> *fd_to_close) const {
        if (type == NONE) {
            return 0;
        }
        int flags, mode = 0;

        switch (type) {
            case OUT:
                flags = O_WRONLY | O_CREAT | O_TRUNC;
                mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                break;
            case OUT_APPEND:
                flags = O_WRONLY | O_CREAT | O_APPEND;
                mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
                break;
            case IN:
                flags = O_RDONLY;
                break;
            default:
                return 0;
        }

        int in_fd, out_fd;
        in_fd = in.open_redirect(fd_to_close, flags, mode);
        out_fd = out.open_redirect(fd_to_close, flags, mode);

        if (in_fd != -1 && out_fd != -1) {
            if (dup2(out_fd, in_fd) == -1) {
                msh_error("cannot redirect: " + std::string(strerror(errno)));
                return 1;
            }
        }

        if (both_err_out) {
            if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
                msh_error("cannot redirect: " + std::string(strerror(errno)));
                return 1;
            }
        }
        return 0;
    }
};


#endif //MYSHELL_MSH_REDIRECT_H
