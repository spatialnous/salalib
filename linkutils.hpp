// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "latticemap.hpp"

#include "genlib/exceptions.hpp"

#include <vector>

namespace sala {

    class InvalidLinkException : public genlib::BaseException {
      public:
        InvalidLinkException(std::string message) : genlib::BaseException(std::move(message)) {}
    };
    std::vector<PixelRefPair> pixelateMergeLines(const std::vector<Line4f> &mergeLines,
                                                 LatticeMap &currentMap);
    void mergePixelPairs(const std::vector<PixelRefPair> &links, LatticeMap &currentMap);
    void unmergePixelPairs(const std::vector<PixelRefPair> &links, LatticeMap &currentMap);
    std::vector<SimpleLine> getMergedPixelsAsLines(LatticeMap &currentMap);
} // namespace sala
