// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "edgeu.hpp"

// TODO: Unused function
// EdgeU is used for polygon clipping to viewports

// are a,b,c in ccw order (true) or cw order (false)
bool EdgeU::ccwEdgeU(const EdgeU &b, const EdgeU &c) const {
    bool ccw = false;
    if (c.edge > edge || (c.edge == edge && c.u > u)) {
        if (b.edge > edge || (b.edge == edge && b.u > u)) {
            if (b.edge < c.edge || (b.edge == c.edge && b.u < c.u)) {
                ccw = true;
            }
        }
    } else {
        if (b.edge > edge || (b.edge == edge && b.u > u)) {
            ccw = true;
        } else if (b.edge < c.edge || (b.edge == c.edge && b.u < c.u)) {
            ccw = true;
        }
    }
    return ccw;
}
