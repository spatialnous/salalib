// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface: the meta graph loads and holds all sorts of arbitrary data...

#include "fileproperties.hpp"
#include "shapemap.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

struct MetaGraph {
    Region4f region;
    std::string name = "";
    FileProperties fileProperties;
    int version = -1;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    MetaGraph(std::string nameIn = "")
        : region(), name(std::move(nameIn)), fileProperties(), version(-1), _padding0(0) {

        // <- if unsaved, file version is -1
    }

    void updateParentRegions(ShapeMap &shapeMap);
};
