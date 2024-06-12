// Copyright (C) 2011-2012, Tasos Varoudis

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <istream>
#include <map>
#include <ostream>
#include <vector>
/////////////////////////////////////////////////////////////////////////////

// Additional for segment analysis

struct SegmentRef {
    char dir;
    // padding the remaining three bytes behind the char
    char pad1 : 8;
    short pad2 : 16;
    int ref;
    SegmentRef(char d = 0, int r = -1) : pad1(0), pad2(0) {
        dir = d;
        ref = r;
    }
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
    SegmentData(char d = 0, int r = -1, SegmentRef p = SegmentRef(), int sd = 0, float md = 0.0f,
                unsigned int cv = 0xffffffff) {
        dir = d;
        ref = r;
        previous = p;
        segdepth = sd;
        metricdepth = md;
        coverage = cv;
    }
    SegmentData(SegmentRef ref, SegmentRef p = SegmentRef(), int sd = 0, float md = 0.0f,
                unsigned int cv = 0xffffffff)
        : SegmentRef(ref) {
        previous = p;
        segdepth = sd;
        metricdepth = md;
        coverage = cv;
    }
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
    int m_segment_axialref;
    // use one or other of these
    std::vector<size_t> m_connections;
    //
    std::map<SegmentRef, float> m_back_segconns;
    std::map<SegmentRef, float> m_forward_segconns;
    //
    Connector(int axialref = -1) { m_segment_axialref = axialref; }
    void clear() {
        m_connections.clear();
        m_back_segconns.clear();
        m_forward_segconns.clear();
    }
    //
    bool read(std::istream &stream);
    bool write(std::ofstream &stream);
    //
    // Cursor extras
    enum { CONN_ALL, SEG_CONN_ALL, SEG_CONN_FW, SEG_CONN_BK };

    // PK: These functions have been stripped of state and left
    // here for salaprogram which seems to be the only place they
    // are used. salaprogram seems to also be the only place where
    // the last two modes (SEG_CONN_FW, SEG_CONN_BK) are used
    size_t count(int mode = CONN_ALL) const;
    int getConnectedRef(int cursor, int mode = CONN_ALL) const;
    int direction(int cursor, int mode = SEG_CONN_ALL) const;
    float weight(int cursor, int mode = SEG_CONN_ALL) const;
};
