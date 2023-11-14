//
// Created by andrew on 11/8/23.
//
/**
 * @file
 * @brief Redirects related utilities.
 */

#include "internal/msh_redirects.h"
#include "types/msh_exception.h"

#include <algorithm>


/**
 * @brief Parse redirects from command's tokens.
 *
 * The processing is done only on the tokens with the REDIRECT flag set.
 *
 * The following redirects are supported:
 * <li> Redirecting output:<br>
 * `n>word` - Open file `word` for writing on file descriptor `n`.
 * If `n` is omitted, it defaults to 1.
 *
 * <li> Appending output:<br>
 * `n>>word` - Open file `word` for appending on file descriptor `n`.
 * If `n` is omitted, it defaults to 1.
 *
 * <li> Redirecting input:<br>
 * `n\<word` - Open file `word` for reading on file descriptor `n`.
 * If `n` is omitted, it defaults to 0.
 *
 * <li> Redirecting output and error:<br>
 * `&>word` - Redirect both standard output and standard error to file `word`.
 * Equivalent to `>word 2>&1` and `>&word`.
 *
 * <li> Appending output and error:<br>
 * `&>>word` - Append both standard output and standard error to file `word`.
 * Semantically equivalent to `>>word 2>&1`.
 *
 * <li> Duplicating file descriptors:<br>
 * `n\<&word` - The file descriptor `n` is made to be a copy of the descriptor
 * specified by `word`. If `word` doesn't specify a descriptor, the redirection
 * is ill-formed due to ambiguity. If `n` is not specified, the standard input
 * (file descriptor 0) is used. If descriptor specified by `word` is not correct,
 * the redirection error occurs. <br>
 * `n>&word` - Used for duplicating output file descriptors. If `n` is not
 * specified, the standard output (file descriptor 1) is used.
 * If `word` doesn't specify a descriptor, it is interpreted as a filename to
 * open. If the file descriptor specified by `word` is not correct, the
 * redirection error occurs. If `n` is omitted, and `word` does not specify a
 * file descriptor, the redirect is equivalent to `&>word`.
 *
 * @param tokens  The tokens to parse.
 * @return redirects_t The structure containing the parsed redirects.
 */
redirects_t parse_redirects(tokens_t &tokens) {
    using std::ranges::find_if;
    using std::ranges::all_of;

    auto all_digits = [](std::string_view s) {
        return all_of(s.begin(), s.end(), [](char c) { return std::isdigit(c); });
    };

    redirects_t redirects;
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        auto token = *it;
        if (!token.get_flag(REDIRECT)) {
            continue;
        }

        redirect r(token);

        auto is_amp_x = token.type == TokenType::AMP_OUT || token.type == TokenType::AMP_APPEND;
        if (auto prev = (it - 1); it != tokens.begin() && !is_amp_x) {
            if (prev->get_flag(WORD_LIKE) && all_digits(prev->value)) {
                int fd;
                bool is_fd = true;
                try {
                    fd = std::stoi(prev->value);
                } catch (const std::exception &) {
                    is_fd = false;
                }

                if (is_fd) {
                    r.in.fd = fd;
                    tokens.erase(prev);
                    --it;
                    token = *it;
                }
            }
        }

        std::input_iterator auto
        next_word = find_if(it + 1, tokens.end(),[](Token const &t) { return t.get_flag(WORD_LIKE); });

        if (next_word == tokens.end()) {
            throw msh_exception("parse error near " + token.value, INTERNAL_ERROR);
        }

        auto is_x_amp = token.type == TokenType::OUT_AMP || token.type == TokenType::IN_AMP;
        if (is_x_amp) {
            if (all_digits(next_word->value)) {
                int fd;
                try {
                    fd = std::stoi(next_word->value);
                } catch (const std::exception &) {
                    throw msh_exception("Invalid file descriptor: " + next_word->value, INTERNAL_ERROR);
                }

                r.out.fd = fd;
            } else if (token.type == TokenType::OUT_AMP) {
                r.both_err_out = true;
                r.out.path = next_word->value;
            } else {
                throw msh_exception(next_word->value + ": ambiguous redirect", INTERNAL_ERROR);
            }
        } else {
            r.out.path = next_word->value;
        }

        tokens.erase(next_word);
        redirects.push_back(r);
    }

    return redirects;
}
