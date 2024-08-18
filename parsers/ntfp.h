// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/comm.h"
#include "genlib/p2dpoly.h"

#include <string>
#include <vector>

struct NtfPoint {
    int chars;
    int a = 0;
    int b = 0;
    NtfPoint(int chars = 10) // apparently 10 is NTF default
        : chars(chars) {}
    int parse(const std::string &token, bool secondhalf = false);
};

class NtfGeometry {
  public:
    std::vector<Line> lines;
    NtfGeometry() { ; }
};

class NtfLayer {
    friend class NtfMap;

  protected:
    std::string m_name;
    size_t m_lineCount;

  public:
    int pad1 : 32;
    std::vector<NtfGeometry> geometries;
    NtfLayer(const std::string &name = std::string()) {
        m_name = name;
        m_lineCount = 0;
    }
    size_t getLineCount() { return m_lineCount; }
    std::string getName() { return m_name; }
};

class NtfMap {
  public:
    std::vector<NtfLayer> layers;
    enum { NTF_UNKNOWN, NTF_LANDLINE, NTF_MERIDIAN };

  protected:
    NtfPoint m_offset; // note: in metres
    QtRegion m_region; // made in metres, although points are in cm
    int m_lineCount;

  public:
    NtfMap() { ; }
    Line makeLine(const NtfPoint &a, const NtfPoint &b);

    void open(const std::vector<std::string> &fileset, Communicator *comm);
    const QtRegion &getRegion() const { return m_region; }
    int getLineCount() const { return m_lineCount; }

  protected:
    void fitBounds(const Line &li);
    void addGeom(size_t layerIdx, NtfGeometry &geom);
};
