// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/region4f.h"

#include <map>
#include <string>
#include <vector>

namespace depthmapX {
    typedef std::vector<std::string> ColumnData;
    typedef std::map<std::string, ColumnData> Table;

    class Polyline : public Region4f {
      public:
        std::vector<Point2f> vertices;
        bool closed = false;
#ifdef USE_EXPLICIT_PADDING
        unsigned : 3 * 8; // padding
        unsigned : 4 * 8; // padding
#endif
        Polyline(std::vector<Point2f> verticesIn, bool isClosed)
            : vertices(verticesIn), closed(isClosed) {}
    };

    enum ImportType { DRAWINGMAP, DATAMAP };

    enum ImportFileType { CSV, TSV, DXF, CAT, RT1, NTF };
} // namespace depthmapX
