// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <ostream>
#include <vector>
/////////////////////////////////////////////////////////////////////////////

// Additional for segment analysis

struct SegmentRef {
    int8_t dir;
    // padding the remaining three bytes behind the char
    int8_t pad1 : 8;
    short pad2 : 16;
    int ref;
    SegmentRef(int8_t d = 0, int r = -1) : dir(d), pad1(0), pad2(0), ref(r) {}
};
// note, the dir is only a direction indicator, the ref should always be unique
inline bool operator<(SegmentRef a, SegmentRef b) { return a.ref < b.ref; }
inline bool operator>(SegmentRef a, SegmentRef b) { return a.ref > b.ref; }
inline bool operator==(SegmentRef a, SegmentRef b) { return a.ref == b.ref; }
inline bool operator!=(SegmentRef a, SegmentRef b) { return a.ref != b.ref; }

// used during angular analysis
struct SegmentData : public SegmentRef {
    SegmentRef previous;
    int segdepth;
    float metricdepth;
    unsigned int coverage;
    SegmentData(int8_t d = 0, int r = -1, SegmentRef p = SegmentRef(), int sd = 0, float md = 0.0f,
                unsigned int cv = 0xffffffff)
        : previous(p), segdepth(sd), metricdepth(md), coverage(cv) {
        dir = d;
        ref = r;
    }
    SegmentData(SegmentRef sref, SegmentRef p = SegmentRef(), int sd = 0, float md = 0.0f,
                unsigned int cv = 0xffffffff)
        : SegmentRef(sref), previous(p), segdepth(sd), metricdepth(md), coverage(cv) {}
    friend bool operator<(SegmentData a, SegmentData b);
    friend bool operator>(SegmentData a, SegmentData b);
    friend bool operator==(SegmentData a, SegmentData b);
    friend bool operator!=(SegmentData a, SegmentData b);
};
// note, these are stored in reverse metric depth order (i.e., metric shorter paths are taken off
// the end of the list first)
inline bool operator<(SegmentData a, SegmentData b) { return a.metricdepth > b.metricdepth; }
inline bool operator>(SegmentData a, SegmentData b) { return a.metricdepth < b.metricdepth; }
inline bool operator==(SegmentData a, SegmentData b) { return a.metricdepth == b.metricdepth; }
inline bool operator!=(SegmentData a, SegmentData b) { return a.metricdepth != b.metricdepth; }

///////////////////////////////////////////////////////////////////////////

// Main connector class for segments, convex spaces or axial lines

struct Connector {
    //  if this is a segment, this is the key for the axial line:
    int segmentAxialref;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    // use one or other of these
    std::vector<size_t> connections;
    //
    std::map<SegmentRef, float> backSegconns;
    std::map<SegmentRef, float> forwardSegconns;
    //
    Connector(int axialref = -1)
        : segmentAxialref(axialref), _padding0(0), connections(), backSegconns(),
          forwardSegconns() {}
    void clear() {
        connections.clear();
        backSegconns.clear();
        forwardSegconns.clear();
    }
    //
    bool read(std::istream &stream);
    bool write(std::ostream &stream) const;
    //
    // Cursor extras
    enum { CONN_ALL, SEG_CONN_ALL, SEG_CONN_FW, SEG_CONN_BK };

    // PK: These functions have been stripped of state and left
    // here for salaprogram which seems to be the only place they
    // are used. salaprogram seems to also be the only place where
    // the last two modes (SEG_CONN_FW, SEG_CONN_BK) are used
    size_t count(int mode = CONN_ALL) const;
    int getConnectedRef(const int cursor, const int mode = CONN_ALL) const;
    int direction(const int cursor, const int mode = SEG_CONN_ALL) const;
    float weight(const int cursor, const int mode = SEG_CONN_ALL) const;
};
