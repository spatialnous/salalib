// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pointmap.hpp"

#include "genlib/exceptions.hpp"

#include <vector>

namespace sala {

    class InvalidLinkException : public genlib::BaseException {
      public:
        InvalidLinkException(std::string message) : genlib::BaseException(std::move(message)) {}
    };
    std::vector<PixelRefPair> pixelateMergeLines(const std::vector<Line4f> &mergeLines,
                                                 PointMap &currentMap);
    void mergePixelPairs(const std::vector<PixelRefPair> &links, PointMap &currentMap);
    void unmergePixelPairs(const std::vector<PixelRefPair> &links, PointMap &currentMap);
    std::vector<SimpleLine> getMergedPixelsAsLines(PointMap &currentMap);
} // namespace sala
