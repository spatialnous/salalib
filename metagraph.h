// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface: the meta graph loads and holds all sorts of arbitrary data...

#include "fileproperties.h"
#include "shapemap.h"

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

struct MetaGraph {
    Region4f region;
    std::string name = "";
    FileProperties fileProperties;
    int version = -1;

    MetaGraph(std::string nameIn = "") : name(nameIn), version(-1) {

        // <- if unsaved, file version is -1
    }

    void updateParentRegions(ShapeMap &shapeMap);
};
