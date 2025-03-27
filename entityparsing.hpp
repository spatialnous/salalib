// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "isovistdef.hpp"

#include "genlib/exceptions.hpp"
#include "genlib/line4f.hpp"

#include <string>
#include <vector>

namespace EntityParsing {

    class EntityParseException : public depthmapX::BaseException {
      public:
        EntityParseException(std::string message) : depthmapX::BaseException(std::move(message)) {}
    };

    std::vector<std::string> split(const std::string &s, char delim);
    std::vector<Line4f> parseLines(std::istream &stream, char delimiter);
    std::vector<Point2f> parsePoints(std::istream &stream, char delimiter);
    Point2f parsePoint(const std::string &point, char delimiter = ',');
    std::vector<IsovistDefinition> parseIsovists(std::istream &stream, char delimiter);
    IsovistDefinition parseIsovist(const std::string &isovist);
    std::vector<std::pair<int, int>> parseRefPairs(std::istream &stream, char delimiter);
} // namespace EntityParsing
