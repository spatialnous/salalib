// SPDX-FileCopyrightText: 2017 Christian Sailer
// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <exception>
#include <string>

namespace depthmapX {
    class BaseException : public std::exception {
      public:
        BaseException() {}
        BaseException(std::string message) : _message(message) {}
        virtual const char *what() const noexcept { return _message.c_str(); }

      private:
        std::string _message;
    };

    class RuntimeException : public BaseException {
      public:
        RuntimeException(std::string message) : BaseException(message) {}
    };
} // namespace depthmapX
