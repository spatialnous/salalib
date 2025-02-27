// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "shapemap.hpp"

#include <string>

struct ShapeMapGroupData {
    std::string name; // <- file name
    Region4f region;

    ShapeMapGroupData(const std::string &nameIn = std::string()) : name(nameIn), region() {}

    bool readInNameAndRegion(std::istream &stream);
    bool writeOutNameAndRegion(std::ostream &stream) const;

    static std::tuple<std::vector<ShapeMap>, std::vector<std::tuple<bool, bool, int>>>
    readSpacePixels(std::istream &stream);

    static std::vector<std::pair<std::reference_wrapper<const ShapeMap>, int>> getAsRefMaps(
        const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>> &drawingFiles) {
        std::vector<std::pair<std::reference_wrapper<const ShapeMap>, int>> maps;
        for (auto &mapGroup : drawingFiles) {
            int j = 0;
            for (const auto &map : mapGroup.second) {
                maps.push_back(std::make_pair(std::ref(map), j));
            }
        }
        return maps;
    }
};
