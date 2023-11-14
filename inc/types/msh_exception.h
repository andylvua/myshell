//
// Created by andrew on 11/9/23.
//

#ifndef MYSHELL_MSH_EXCEPTION_H
#define MYSHELL_MSH_EXCEPTION_H

#include "internal/msh_error.h"

#include <exception>
#include <string>

/**
 * @brief Internal exception class.
 *
 * @see msh_error
 */
class msh_exception : public std::exception {
public:
    explicit msh_exception(std::string message, msh_err err = UNKNOWN_ERROR) :
    message(std::move(message)), err(err) {}

    [[nodiscard]] const char *what() const noexcept override {
        return message.c_str();
    }

    [[nodiscard]] msh_err code() const noexcept {
        return err;
    }

private:
    std::string message;
    msh_err err;
};

#endif //MYSHELL_MSH_EXCEPTION_H
