// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
struct TopoMetSegmentRef {
    double dist;
    int ref;
    int dir;
    int previous;
    bool done;
#ifdef USE_EXPLICIT_PADDING
    unsigned : 3 * 8; // padding
#endif
    TopoMetSegmentRef(int r = -1, int d = -1, double di = 0.0, int p = -1)
        : dist(di), ref(r), dir(d), previous(p), done(false) {}
};

// should be double not float!

struct TopoMetSegmentChoice {
    double choice;
    double wchoice;
    TopoMetSegmentChoice() : choice(0.0), wchoice(0.0) {}
};

struct SegInfo {
    double length;
    int layer;
#ifdef USE_EXPLICIT_PADDING
    unsigned : 4 * 8; // padding
#endif
    SegInfo() : length(0.0f), layer(0) {}
};
