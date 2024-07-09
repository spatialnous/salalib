// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

// each pixel has various lists of information:

struct ShapeRef {
    enum { SHAPE_REF_NULL = 0xFFFFFFFF };
    enum { SHAPE_L = 0x01, SHAPE_B = 0x02, SHAPE_R = 0x04, SHAPE_T = 0x08 };
    enum { SHAPE_EDGE = 0x0f, SHAPE_INTERNAL_EDGE = 0x10, SHAPE_CENTRE = 0x20, SHAPE_OPEN = 0x40 };
    unsigned char m_tags;
    unsigned int m_shape_ref;
    std::vector<short> m_polyrefs;
    ShapeRef(unsigned int sref = SHAPE_REF_NULL, unsigned char tags = 0x00) {
        m_shape_ref = sref;
        m_tags = tags;
    }
    friend bool operator==(const ShapeRef &a, const ShapeRef &b);
    friend bool operator!=(const ShapeRef &a, const ShapeRef &b);
    friend bool operator<(const ShapeRef &a, const ShapeRef &b);
    friend bool operator>(const ShapeRef &a, const ShapeRef &b);
};
inline bool operator==(const ShapeRef &a, const ShapeRef &b) {
    return a.m_shape_ref == b.m_shape_ref;
}
inline bool operator!=(const ShapeRef &a, const ShapeRef &b) {
    return a.m_shape_ref != b.m_shape_ref;
}
inline bool operator<(const ShapeRef &a, const ShapeRef &b) {
    return a.m_shape_ref < b.m_shape_ref;
}
inline bool operator>(const ShapeRef &a, const ShapeRef &b) {
    return a.m_shape_ref > b.m_shape_ref;
}

struct ShapeRefHash {
  public:
    std::size_t operator()(const ShapeRef &shapeRef) const { return shapeRef.m_shape_ref; }
};
