// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>

// each pixel has various lists of information:

struct ShapeRef {
    enum { SHAPE_REF_NULL = 0xFFFFFFFF };
    enum { SHAPE_L = 0x01, SHAPE_B = 0x02, SHAPE_R = 0x04, SHAPE_T = 0x08 };
    enum { SHAPE_EDGE = 0x0f, SHAPE_INTERNAL_EDGE = 0x10, SHAPE_CENTRE = 0x20, SHAPE_OPEN = 0x40 };
    uint8_t tags;

  private:
    [[maybe_unused]] unsigned _padding0 : 3 * 8;

  public:
    unsigned int shapeRef;
    std::vector<short> polyrefs;
    ShapeRef(unsigned int sref = SHAPE_REF_NULL, uint8_t tagsIn = 0x00)
        : tags(tagsIn), _padding0(0), shapeRef(sref), polyrefs() {}
    friend bool operator==(const ShapeRef &a, const ShapeRef &b);
    friend bool operator!=(const ShapeRef &a, const ShapeRef &b);
    friend bool operator<(const ShapeRef &a, const ShapeRef &b);
    friend bool operator>(const ShapeRef &a, const ShapeRef &b);
};
inline bool operator==(const ShapeRef &a, const ShapeRef &b) { return a.shapeRef == b.shapeRef; }
inline bool operator!=(const ShapeRef &a, const ShapeRef &b) { return a.shapeRef != b.shapeRef; }
inline bool operator<(const ShapeRef &a, const ShapeRef &b) { return a.shapeRef < b.shapeRef; }
inline bool operator>(const ShapeRef &a, const ShapeRef &b) { return a.shapeRef > b.shapeRef; }

struct ShapeRefHash {
  public:
    std::size_t operator()(const ShapeRef &shapeRef) const { return shapeRef.shapeRef; }
};
