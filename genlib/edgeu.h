// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// used for clipping of polygons to regions

struct EdgeU {
    int edge;
#ifdef USE_EXPLICIT_PADDING
    unsigned : 4 * 8; // padding
#endif
    double u;
    EdgeU(int e = -1, double nu = 0.0) : edge(e), u(nu) {}
    EdgeU(const EdgeU &eu) : edge(eu.edge), u(eu.u) {}
    bool ccwEdgeU(const EdgeU &b, const EdgeU &c) const;
};
