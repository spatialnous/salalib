// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "pointdata.h"

#include "genlib/exceptions.h"
#include "genlib/p2dpoly.h"

#include <vector>

namespace depthmapX {

    class InvalidLinkException : public depthmapX::BaseException {
      public:
        InvalidLinkException(std::string message) : depthmapX::BaseException(message) {}
    };
    std::vector<PixelRefPair> pixelateMergeLines(const std::vector<Line> &mergeLines,
                                                 PointMap &currentMap);
    void mergePixelPairs(const std::vector<PixelRefPair> &links, PointMap &currentMap);
    void unmergePixelPairs(const std::vector<PixelRefPair> &links, PointMap &currentMap);
    std::vector<SimpleLine> getMergedPixelsAsLines(PointMap &currentMap);
} // namespace depthmapX
