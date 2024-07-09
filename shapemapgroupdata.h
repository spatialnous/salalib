// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/shapemap.h"

#include "genlib/p2dpoly.h"

#include <string>

class ShapeMapGroupData {
    std::string m_name; // <- file name
    QtRegion m_region;

  public:
    ShapeMapGroupData(const std::string &name = std::string()) : m_name(name){};
    void setName(const std::string &name) { m_name = name; }
    const std::string &getName() const { return m_name; }

    bool readInName(std::istream &stream);
    bool writeOutName(std::ostream &stream) const;

    static std::tuple<std::vector<ShapeMap>, std::vector<std::tuple<bool, bool, int>>>
    readSpacePixels(std::istream &stream);
};
