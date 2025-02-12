// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "entityparsing.hpp"

#include "genlib/stringutils.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <sstream>

namespace EntityParsing {

    std::vector<Line4f> parseLines(std::istream &stream, char delimiter = '\t') {

        std::vector<Line4f> lines;

        std::string inputline;
        std::getline(stream, inputline);

        std::vector<std::string> strings = dXstring::split(inputline, delimiter);

        if (strings.size() < 4) {
            throw EntityParseException("Badly formatted header (should contain x1, y1, x2 and y2)");
        }

        size_t i;
        for (i = 0; i < strings.size(); i++) {
            if (!strings[i].empty()) {
                dXstring::toLower(strings[i]);
                // strings[i].ltrim('\"');
                // strings[i].rtrim('\"');
            }
        }

        int x1col = -1, y1col = -1, x2col = -1, y2col = -1;
        for (i = 0; i < strings.size(); i++) {
            if (strings[i] == "x1") {
                x1col = static_cast<int>(i);
            } else if (strings[i] == "x2") {
                x2col = static_cast<int>(i);
            } else if (strings[i] == "y1") {
                y1col = static_cast<int>(i);
            } else if (strings[i] == "y2") {
                y2col = static_cast<int>(i);
            }
        }

        if (x1col == -1 || y1col == -1 || x2col == -1 || y2col == -1) {
            throw EntityParseException("Badly formatted header (should contain x1, y1, x2 and y2)");
        }

        Point2f p1, p2;

        while (!stream.eof()) {
            std::getline(stream, inputline);
            if (!inputline.empty()) {
                strings = dXstring::split(inputline, delimiter);
                if (!strings.size()) {
                    continue;
                }
                if (strings.size() < 4) {
                    std::stringstream message;
                    message << "Error parsing line: " << inputline << std::flush;
                    throw EntityParseException(message.str().c_str());
                }
                for (i = 0; i < strings.size(); i++) {
                    if (static_cast<int>(i) == x1col) {
                        p1.x = std::atof(strings[i].c_str());
                    } else if (static_cast<int>(i) == y1col) {
                        p1.y = std::atof(strings[i].c_str());
                    } else if (static_cast<int>(i) == x2col) {
                        p2.x = std::atof(strings[i].c_str());
                    } else if (static_cast<int>(i) == y2col) {
                        p2.y = std::atof(strings[i].c_str());
                    }
                }
                lines.push_back(Line4f(p1, p2));
            }
        }
        return lines;
    }

    std::vector<Point2f> parsePoints(std::istream &stream, char delimiter = '\t') {

        std::vector<Point2f> points;

        std::string inputline;
        std::getline(stream, inputline);

        std::vector<std::string> strings = dXstring::split(inputline, delimiter);

        if (strings.size() < 2) {
            throw EntityParseException("Badly formatted header (should contain x and y)");
        }

        size_t i;
        for (i = 0; i < strings.size(); i++) {
            if (!strings[i].empty()) {
                dXstring::toLower(strings[i]);
                // strings[i].ltrim('\"');
                // strings[i].rtrim('\"');
            }
        }

        int xcol = -1, ycol = -1;
        for (i = 0; i < strings.size(); i++) {
            if (strings[i] == "x") {
                xcol = static_cast<int>(i);
            } else if (strings[i] == "y") {
                ycol = static_cast<int>(i);
            }
        }

        if (xcol == -1 || ycol == -1) {
            throw EntityParseException("Badly formatted header (should contain x and y)");
        }

        Point2f p;

        while (!stream.eof()) {
            std::getline(stream, inputline);
            if (!inputline.empty()) {
                strings = dXstring::split(inputline, delimiter);
                if (!strings.size()) {
                    continue;
                }
                if (strings.size() < 2) {
                    std::stringstream message;
                    message << "Error parsing line: " << inputline << std::flush;
                    throw EntityParseException(message.str().c_str());
                }
                for (i = 0; i < strings.size(); i++) {
                    if (static_cast<int>(i) == xcol) {
                        p.x = std::atof(strings[i].c_str());
                    } else if (static_cast<int>(i) == ycol) {
                        p.y = std::atof(strings[i].c_str());
                    }
                }
                points.push_back(p);
            }
        }
        return points;
    }

    Point2f parsePoint(const std::string &point, char delimiter) {
        std::vector<std::string> strings = dXstring::split(point, delimiter);

        if (strings.size() != 2) {
            std::stringstream message;
            message << "Badly formatted point data, should be <number>" << delimiter
                    << "<number>, was " << point << std::flush;
            throw EntityParseException(message.str());
        }
        return Point2f(atof(strings[0].c_str()), atof(strings[1].c_str()));
    }

    std::vector<IsovistDefinition> parseIsovists(std::istream &stream, char delimiter) {
        std::vector<IsovistDefinition> isovists;

        std::string inputline;
        std::getline(stream, inputline);

        std::vector<std::string> strings = dXstring::split(inputline, delimiter);

        if (strings.size() < 2) {
            throw EntityParseException("Badly formatted header (should contain x, y, can also have "
                                       "angle and viewangle for partial isovists)");
        }

        size_t i;
        for (i = 0; i < strings.size(); i++) {
            if (!strings[i].empty()) {
                dXstring::toLower(strings[i]);
            }
        }

        int xcol = -1, ycol = -1, anglecol = -1, viewcol = -1;
        for (i = 0; i < strings.size(); i++) {
            if (strings[i] == "x") {
                xcol = static_cast<int>(i);
            } else if (strings[i] == "y") {
                ycol = static_cast<int>(i);
            } else if (strings[i] == "angle") {
                anglecol = static_cast<int>(i);
            } else if (strings[i] == "viewangle") {
                viewcol = static_cast<int>(i);
            }
        }

        if (xcol == -1 || ycol == -1) {
            throw EntityParseException("Badly formatted header (should contain x and y, might also "
                                       "have angle and viewangle for partial isovists)");
        }

        bool partialIsovists = anglecol != -1 && viewcol != -1;
        int maxCol = std::max({xcol, ycol, anglecol, viewcol});
        while (!stream.eof()) {
            std::getline(stream, inputline);
            if (!inputline.empty()) {
                strings = dXstring::split(inputline, delimiter);
                if (!strings.size()) {
                    continue;
                }
                if (static_cast<int>(strings.size()) <= maxCol) {
                    std::stringstream message;
                    message << "Error parsing line: " << inputline << std::flush;
                    throw EntityParseException(message.str().c_str());
                }

                double x = std::atof(strings[static_cast<size_t>(xcol)].c_str());
                double y = std::atof(strings[static_cast<size_t>(ycol)].c_str());

                if (partialIsovists) {
                    double angle =
                        std::atof(strings[static_cast<size_t>(anglecol)].c_str()) / 180.0 * M_PI;
                    double viewAngle =
                        std::atof(strings[static_cast<size_t>(viewcol)].c_str()) / 180.0 * M_PI;
                    isovists.push_back(IsovistDefinition(x, y, angle, viewAngle));
                } else {
                    isovists.push_back(IsovistDefinition(x, y));
                }
            }
        }
        return isovists;
    }

    IsovistDefinition parseIsovist(const std::string &isovist) {
        auto parts = dXstring::split(isovist, ',');
        if (parts.size() == 2) {
            return IsovistDefinition(std::atof(parts[0].c_str()), std::atof(parts[1].c_str()));
        } else if (parts.size() == 4) {
            double angle = std::atof(parts[2].c_str()) / 180.0 * M_PI;
            double viewAngle = std::atof(parts[3].c_str()) / 180.0 * M_PI;
            return IsovistDefinition(std::atof(parts[0].c_str()), std::atof(parts[1].c_str()),
                                     angle, viewAngle);
        }
        std::stringstream message;
        message << "Failed to parse '" << isovist << "' to an isovist definition";
        throw EntityParseException(message.str());
    }

    std::vector<std::pair<int, int>> parseRefPairs(std::istream &stream, char delimiter = '\t') {

        std::vector<std::pair<int, int>> pairs;

        std::string inputline;
        std::getline(stream, inputline);

        std::vector<std::string> strings = dXstring::split(inputline, delimiter);

        if (strings.size() < 2) {
            throw EntityParseException("Badly formatted header (should contain reffrom and refto)");
        }

        size_t i;
        for (i = 0; i < strings.size(); i++) {
            if (!strings[i].empty()) {
                dXstring::toLower(strings[i]);
                // strings[i].ltrim('\"');
                // strings[i].rtrim('\"');
            }
        }

        int fromcol = -1, tocol = -1;
        for (i = 0; i < strings.size(); i++) {
            if (strings[i] == "reffrom") {
                fromcol = static_cast<int>(i);
            } else if (strings[i] == "refto") {
                tocol = static_cast<int>(i);
            }
        }

        if (fromcol == -1 || tocol == -1) {
            throw EntityParseException("Badly formatted header (should contain reffrom and refto)");
        }

        while (!stream.eof()) {
            std::getline(stream, inputline);
            if (!inputline.empty()) {
                strings = dXstring::split(inputline, delimiter);
                if (!strings.size()) {
                    continue;
                }
                if (strings.size() < 2) {
                    std::stringstream message;
                    message << "Error parsing line: " << inputline << std::flush;
                    throw EntityParseException(message.str().c_str());
                }
                int from = -1, to = -1;
                for (i = 0; i < strings.size(); i++) {
                    if (i == static_cast<size_t>(fromcol)) {
                        from = std::atoi(strings[i].c_str());
                    } else if (i == static_cast<size_t>(tocol)) {
                        to = std::atoi(strings[i].c_str());
                    }
                }
                pairs.push_back(std::make_pair(from, to));
            }
        }
        return pairs;
    }

} // namespace EntityParsing
