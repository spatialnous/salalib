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
        BaseException(std::string message) : m_message(message) {}
        const char *what() const noexcept override { return m_message.c_str(); }

      private:
        std::string m_message;
    };

    class RuntimeException : public BaseException {
      public:
        RuntimeException(std::string message) : BaseException(message) {}
    };
} // namespace depthmapX
