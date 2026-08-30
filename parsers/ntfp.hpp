// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/comm.hpp"
#include "../genlib/line4f.hpp"

#include <string>
#include <vector>

struct NtfPoint {
    int chars;
    int a = 0;
    int b = 0;
    NtfPoint(int charsIn = 10) // apparently 10 is NTF default
        : chars(charsIn) {}
    int parse(const std::string &token, bool secondhalf = false);
};

class NtfGeometry {
  public:
    std::vector<Line4f> lines;
    NtfGeometry() : lines() {}
};

class NtfLayer {
    friend class NtfMap;

  protected:
    std::string m_name;
    size_t m_lineCount;

  public:
    std::vector<NtfGeometry> geometries;
    NtfLayer(const std::string &name = std::string())
        : m_name(name), m_lineCount(0), geometries() {}
    size_t getLineCount() { return m_lineCount; }
    std::string getName() { return m_name; }
};

class NtfMap {
  public:
    std::vector<NtfLayer> layers;
    enum { NTF_UNKNOWN, NTF_LANDLINE, NTF_MERIDIAN };

  protected:
    int m_lineCount;
    NtfPoint m_offset; // note: in metres
    Region4f m_region; // made in metres, although points are in cm

  public:
    NtfMap() : layers(), m_lineCount(), m_offset(), m_region() {}
    Line4f makeLine(const NtfPoint &a, const NtfPoint &b);

    void open(const std::vector<std::string> &fileset, Communicator *comm);
    const Region4f &getRegion() const { return m_region; }
    int getLineCount() const { return m_lineCount; }

  protected:
    void fitBounds(const Line4f &li);
    void addGeom(size_t layerIdx, NtfGeometry &geom);
};
