// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "importtypedefs.h"
#include "mgraph.h"
#include "parsers/dxfp.h"

#include <map>
#include <vector>

namespace depthmapX {
    bool importFile(MetaGraph &mgraph, std::istream &stream, Communicator *communicator,
                    std::string name, ImportType mapType, ImportFileType fileType);
    bool importTxt(ShapeMap &shapeMap, std::istream &stream, char delimiter);
    depthmapX::Table csvToTable(std::istream &stream, char delimiter);
    std::vector<Line> extractLines(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col,
                                   ColumnData &y2col);
    std::map<int, Line> extractLinesWithRef(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col,
                                            ColumnData &y2col, ColumnData &refcol);
    std::vector<Point2f> extractPoints(ColumnData &x, ColumnData &y);
    std::map<int, Point2f> extractPointsWithRefs(ColumnData &x, ColumnData &y, ColumnData &ref);
    bool importDxfLayer(const DxfLayer &dxfLayer, ShapeMap &shapeMap);
    bool importAttributes(AttributeTable &attributes, std::istream &stream, char delimiter);
} // namespace depthmapX
