//
// Created by andrew on 11/8/23.
//

#include "internal/msh_redirects.h"
#include "types/msh_exception.h"

#include <algorithm>


redirects_t parse_redirects(tokens_t &tokens) {
    using std::ranges::find_if;
    using std::ranges::all_of;

    redirects_t redirects;
    for (auto it = tokens.begin(); it != tokens.end(); ++it) {
        auto token = *it;
        if (!token.get_flag(REDIRECT)) {
            continue;
        }

        redirect r(token);

        if (auto prev = (it - 1); it != tokens.begin() && token.type != TokenType::AMP_OUT) {
            if (prev->get_flag(WORD_LIKE) &&
            all_of(prev->value, [](char c) { return std::isdigit(c); })) {
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

        if (token.type == TokenType::IN_AMP || token.type == TokenType::OUT_AMP) {
            if (all_of(next_word->value, [](char c) { return std::isdigit(c); })) {
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
