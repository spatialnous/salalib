// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/p2dpoly.h"

#include <map>
#include <string>
#include <vector>

namespace depthmapX {
    typedef std::vector<std::string> ColumnData;
    typedef std::map<std::string, ColumnData> Table;

    class Polyline : public QtRegion {
      public:
        std::vector<Point2f> m_vertices;
        bool m_closed = false;
        Polyline(std::vector<Point2f> vertices, bool closed)
            : m_vertices(vertices), m_closed(closed) {}
    };

    enum ImportType { DRAWINGMAP, DATAMAP };

    enum ImportFileType { CSV, TSV, DXF };
} // namespace depthmapX
