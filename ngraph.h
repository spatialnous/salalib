// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// ngraph.h

#pragma once

#include "pixelref.h"

#include <iostream>
#include <set>

struct PixelVec {
    PixelVec(const PixelRef start = NoPixel, const PixelRef end = NoPixel) {
        m_start = (int)start;
        m_end = (int)end;
    };
    PixelRef start() const { return m_start; }
    PixelRef end() const { return m_end; } //
    void setStart(PixelRef start) { m_start = start; }
    void setEnd(PixelRef end) { m_end = end; } //
    std::istream &read(std::istream &stream, const char dir);
    std::istream &read(std::istream &stream, const char dir, const PixelVec &context);
    std::ostream &write(std::ostream &stream, const char dir);
    std::ostream &write(std::ostream &stream, const char dir, const PixelVec &context);

  private:
    PixelRef m_start;
    PixelRef m_end;
};

class Bin {
    friend class Node;

  protected:
    // TODO: in new version increase precision?
    unsigned short m_nodeCount;
    float m_distance;
    float m_occDistance;

  public:
    char dir;
    std::vector<PixelVec> pixelVecs;
    Bin() {
        dir = PixelRef::NODIR;
        m_nodeCount = 0;
        m_distance = 0.0f;
        m_occDistance = 0.0f;
    }
    //
    void make(const PixelRefVector &pixels, char onDir);

    int count() const { return m_nodeCount; }
    float distance() const { return m_distance; }
    float occdistance() const { return m_occDistance; }
    //
    void setOccDistance(float d) { m_occDistance = d; }
    //
    bool containsPoint(const PixelRef p) const;
    //
    // Iterator
  protected:
    // Conversion back to old fashioned schema:
    mutable int m_curvec;
    mutable PixelRef m_curpix;

  public:
    void first() const;
    void next() const;
    bool is_tail() const;
    PixelRef cursor() const;
    //
    std::istream &read(std::istream &stream);
    std::ostream &write(std::ostream &stream);
    //
    friend std::ostream &operator<<(std::ostream &stream, const Bin &bin);
};

class Node {
  protected:
    PixelRef m_pixel;
    Bin m_bins[32];

  public:
    // testing some agent stuff:
    std::vector<PixelRef> occlusionBins[32];

  public:
    // Note: this function clears the bins as it goes
    void make(const PixelRef pix, PixelRefVector *bins, float *bin_far_dists, int q_octants);

    bool concaveConnected();
    bool fullyConnected();
    //
    void setPixel(const PixelRef &pixel) { m_pixel = pixel; }
    //
    const Bin &bin(int i) const { return m_bins[i]; }
    Bin &bin(int i) { return m_bins[i]; }
    //
    int count() {
        int c = 0;
        for (int i = 0; i < 32; i++)
            c += m_bins[i].count();
        return c;
    }
    int bincount(int i) { return m_bins[i].count(); }
    float bindistance(int i) { return m_bins[i].distance(); }
    void setbindistances(float bin_dists[32]) {
        for (int i = 0; i < 32; i++)
            m_bins[i].m_distance = bin_dists[i];
    }
    float occdistance(int i) { return m_bins[i].occdistance(); }
    //
    bool containsPoint(const PixelRef p) const;
    //
    //
    // Iterator:
  protected:
    // Conversion back to old fashioned schema:
    mutable int m_curbin;

  public:
    void contents(PixelRefVector &hood) const;
    void first() const;
    void next() const;
    bool is_tail() const;
    PixelRef cursor() const;
    //
    std::istream &read(std::istream &stream);
    std::ostream &write(std::ostream &stream);
    //
    friend std::ostream &operator<<(std::ostream &stream, const Node &node);
};

// Two little helpers:

class PixelRefH : public PixelRef {
  public:
    PixelRefH() : PixelRef() { ; }
    PixelRefH(const PixelRef &p) : PixelRef(p) { ; }
    friend bool operator>(const PixelRefH &a, const PixelRefH &b);
    friend bool operator<(const PixelRefH &a, const PixelRefH &b);
};
inline bool operator>(const PixelRefH &a, const PixelRefH &b) {
    return (a.y > b.y || (a.y == b.y && a.x > b.x));
}
inline bool operator<(const PixelRefH &a, const PixelRefH &b) {
    return (a.y < b.y || (a.y == b.y && a.x < b.x));
}
class PixelRefV : public PixelRef {
  public:
    PixelRefV() : PixelRef() { ; }
    PixelRefV(const PixelRef &p) : PixelRef(p) { ; }
    friend bool operator>(const PixelRefV &a, const PixelRefV &b);
    friend bool operator<(const PixelRefV &a, const PixelRefV &b);
};
inline bool operator>(const PixelRefV &a, const PixelRefV &b) {
    return (a.x > b.x || (a.x == b.x && a.y > b.y));
}
inline bool operator<(const PixelRefV &a, const PixelRefV &b) {
    return (a.x < b.x || (a.x == b.x && a.y < b.y));
}
