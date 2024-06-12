// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "isovistdef.h"

#include "genlib/exceptions.h"
#include "genlib/p2dpoly.h"

#include <iostream>
#include <string>
#include <vector>

namespace EntityParsing {

    class EntityParseException : public depthmapX::BaseException {
      public:
        EntityParseException(std::string message) : depthmapX::BaseException(message) {}
    };

    std::vector<std::string> split(const std::string &s, char delim);
    std::vector<Line> parseLines(std::istream &stream, char delimiter);
    std::vector<Point2f> parsePoints(std::istream &stream, char delimiter);
    Point2f parsePoint(const std::string &point, char delimiter = ',');
    std::vector<IsovistDefinition> parseIsovists(std::istream &stream, char delimiter);
    IsovistDefinition parseIsovist(const std::string &isovist);
    std::vector<std::pair<int, int>> parseRefPairs(std::istream &stream, char delimiter);
} // namespace EntityParsing
