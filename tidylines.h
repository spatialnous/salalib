// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "spacepix.h"

#include "genlib/p2dpoly.h"

// helpers... a class to tidy up ugly maps people may give me...

class TidyLines : public SpacePixel {
  public:
    void tidy(std::vector<Line> &lines, const QtRegion &region);
    void quicktidy(std::map<int, std::pair<Line, int>> &lines, const QtRegion &region);
};
