// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "shapemapgroupdata.h"

#include "salalib/shapemap.h"

#include "genlib/stringutils.h"

bool ShapeMapGroupData::readInName(std::istream &stream) {
    m_name = dXstring::readString(stream);
    stream.read((char *)&m_region, sizeof(m_region));
    if (m_name.empty()) {
        m_name = "<unknown>";
    }
    return true;
}

std::tuple<std::vector<ShapeMap>, std::vector<std::tuple<bool, bool, int>>>
ShapeMapGroupData::readSpacePixels(std::istream &stream) {
    std::vector<ShapeMap> spacePixels;
    std::vector<std::tuple<bool, bool, int>> displayData;
    int count;
    stream.read((char *)&count, sizeof(count));
    for (int i = 0; i < count; i++) {
        spacePixels.emplace_back();
        auto [completed, editable, shown, displayedAttribute] = spacePixels.back().read(stream);
        displayData.emplace_back(editable, shown, displayedAttribute);
    }
    return std::make_tuple(std::move(spacePixels), displayData);
}

bool ShapeMapGroupData::writeOutName(std::ostream &stream) const {
    dXstring::writeString(stream, m_name);
    stream.write((char *)&m_region, sizeof(m_region));
    return true;
}
