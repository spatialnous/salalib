// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/p2dpoly.h"

// this is a helper for cutting polygons to fit a viewport / cropping frame

struct SalaEdgeU : public EdgeU {
    int index;
    bool entry; // or exit
    SalaEdgeU() : EdgeU() {
        index = -1;
        entry = false;
    }
    SalaEdgeU(int i, bool e, const EdgeU &eu) : EdgeU(eu) {
        index = i;
        entry = e;
    }
};
