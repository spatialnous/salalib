// SPDX-FileCopyrightText: 2017 Christian Sailer
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace dXstring {
    std::vector<std::string> split(const std::string &s, char delim, bool skipEmptyTokens = false);
    std::string readString(std::istream &stream);
    void writeString(std::ostream &stream, const std::string &s);
    std::string formatString(double value, const std::string &format = "%+.16le");
    std::string formatString(int value, const std::string &format = "% 16d");
    /// Inplace conversion to lower case
    std::string &toLower(std::string &str);
    void ltrim(std::string &s, char c = ' ');
    void rtrim(std::string &s, char c = ' ');
    void makeInitCaps(std::string &s);
    bool isDouble(const std::string &s);
    template <class T> bool beginsWith(const T &input, const T match) {
        return input.size() >= match.size() && equal(match.begin(), match.end(), input.begin());
    }
    std::istream &safeGetline(std::istream &is, std::string &t);

} // namespace dXstring
