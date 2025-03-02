// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/region4f.hpp"

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

      private:
        [[maybe_unused]] unsigned _padding0 : 3 * 8;
        [[maybe_unused]] unsigned _padding1 : 4 * 8;

      public:
        Polyline(std::vector<Point2f> verticesIn, bool isClosed)
            : vertices(std::move(verticesIn)), closed(isClosed), _padding0(0), _padding1(0) {}
    };

    enum ImportType { DRAWINGMAP, DATAMAP };

    enum ImportFileType { CSV, TSV, DXF, CAT, RT1, NTF };
} // namespace depthmapX
