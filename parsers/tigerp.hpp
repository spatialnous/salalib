// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/comm.hpp"
#include "../genlib/line4f.hpp"

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
    TigerChain() : lines() {}
};

class TigerCategory {
  public:
    std::vector<TigerChain> chains;
    TigerCategory() : chains() {}
};

class TigerMap {
  protected:
    Region4f m_region;
    bool m_init;

  private:
    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    std::map<std::string, TigerCategory> categories;
    TigerMap() : m_region(), m_init(false), _padding0(0), _padding1(0), categories() {}

    void parse(const std::vector<std::string> &fileset, Communicator *communicator);

    Point2f getBottomLeft() { return m_region.bottomLeft; }
    Point2f getTopRight() { return m_region.topRight; }
    Region4f getRegion() { return m_region; }
};
