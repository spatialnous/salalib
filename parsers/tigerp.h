// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/comm.h"
#include "../genlib/line4f.h"

#include <map>
#include <string>
#include <vector>

// look up is the tiger (major) line category:
// string is A1, A2, A3 (road types) or B1, B2 (railroad types)
// C,D etc are not currently parsed, but given the nice file format
// (thank you US Census Bureau!) they can easily be added

class TigerChain {
  public:
    std::vector<Line4f> lines;
    TigerChain() { ; }
};

class TigerCategory {
  public:
    std::vector<TigerChain> chains;
    TigerCategory() { ; }
};

class TigerMap {
  protected:
    Region4f m_region;
    bool m_init;

  public:
    std::map<std::string, TigerCategory> categories;
    TigerMap() : m_init(false) {}

    void parse(const std::vector<std::string> &fileset, Communicator *communicator);

    Point2f getBottomLeft() { return m_region.bottomLeft; }
    Point2f getTopRight() { return m_region.topRight; }
    Region4f getRegion() { return m_region; }
};
