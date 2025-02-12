// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "importtypedefs.hpp"
#include "parsers/dxfp.hpp"
#include "shapemap.hpp"

#include <map>
#include <vector>

namespace depthmapX {
    std::vector<ShapeMap> importFile(std::istream &stream, Communicator *communicator,
                                     std::string name, ImportType mapType, ImportFileType fileType);
    bool importTxt(ShapeMap &shapeMap, std::istream &stream, char delimiter);
    depthmapX::Table csvToTable(std::istream &stream, char delimiter);
    std::vector<Line4f> extractLines(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col,
                                     ColumnData &y2col);
    std::map<int, Line4f> extractLinesWithRef(ColumnData &x1col, ColumnData &y1col,
                                              ColumnData &x2col, ColumnData &y2col,
                                              ColumnData &refcol);
    std::vector<Point2f> extractPoints(ColumnData &x, ColumnData &y);
    std::map<int, Point2f> extractPointsWithRefs(ColumnData &x, ColumnData &y, ColumnData &ref);
    bool importDxfLayer(const DxfLayer &dxfLayer, ShapeMap &shapeMap);
    bool importAttributes(AttributeTable &attributes, std::istream &stream, char delimiter);
    std::vector<ShapeMap> loadCat(std::istream &stream, Communicator *communicator,
                                  ImportType mapType);
    std::vector<ShapeMap> loadRT1(const std::vector<std::string> &fileset,
                                  Communicator *communicator, ImportType mapType);
} // namespace depthmapX
