// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "bspnodetree.hpp"
#include "isovist.hpp"
#include "shapemap.hpp"

namespace IsovistUtils {

    static std::set<std::string> setIsovistData(Isovist &isovist, AttributeTable &table,
                                                AttributeRow &row) {
        std::set<std::string> newColumns;
        auto [centroid, area] = isovist.getCentroidArea();
        auto [driftmag, driftang] = isovist.getDriftData();
        double perimeter = isovist.getPerimeter();

        std::string colText = "Isovist Area";
        auto col = table.getOrInsertColumn(colText);
        newColumns.insert(colText);
        row.setValue(col, static_cast<float>(area));

        colText = "Isovist Compactness";
        col = table.getOrInsertColumn(colText);
        newColumns.insert(colText);
        row.setValue(col, static_cast<float>(4.0 * M_PI * area / (perimeter * perimeter)));

        colText = "Isovist Drift Angle";
        col = table.getOrInsertColumn(colText);
        newColumns.insert(colText);
        row.setValue(col, static_cast<float>(180.0 * driftang / M_PI));

        colText = "Isovist Drift Magnitude";
        col = table.getOrInsertColumn(colText);
        newColumns.insert(colText);
        row.setValue(col, static_cast<float>(driftmag));

        colText = "Isovist Min Radial";
        col = table.getOrInsertColumn(colText);
        newColumns.insert(colText);
        row.setValue(col, static_cast<float>(isovist.getMinRadial()));

        colText = "Isovist Max Radial";
        col = table.getOrInsertColumn(colText);
        newColumns.insert(colText);
        row.setValue(col, static_cast<float>(isovist.getMaxRadial()));

        colText = "Isovist Occlusivity";
        col = table.getOrInsertColumn(colText);
        newColumns.insert(colText);
        row.setValue(col, static_cast<float>(isovist.getOccludedPerimeter()));

        colText = "Isovist Perimeter";
        col = table.getOrInsertColumn(colText);
        newColumns.insert(std::move(colText));
        row.setValue(col, static_cast<float>(perimeter));

        return newColumns;
    }

    static void createIsovistInMap(BSPNodeTree &bspNodeTree, const Region4f &bounds, ShapeMap &map,
                                   const Point2f &p, double startangle, double endangle) {

        Isovist iso;
        iso.makeit(bspNodeTree.getRoot(), p, bounds, startangle, endangle);
        int polyref = map.makePolyShape(iso.getPolygon(), false);
        map.getAllShapes()[polyref].setCentroid(p);

        AttributeTable &table = map.getAttributeTable();
        AttributeRow &row = table.getRow(AttributeKey(polyref));
        setIsovistData(iso, table, row);
    }

    void createIsovistInMap(Communicator *comm, const std::vector<Line4f> &lines,
                            const Region4f &bounds, ShapeMap &map, const Point2f &p,
                            double startangle, double endangle) {
        BSPNodeTree bspNodeTree;
        bspNodeTree.makeNewRoot(false);
        BSPTree::make(comm, 0, lines, bspNodeTree.getRoot());
        createIsovistInMap(bspNodeTree, bounds, map, p, startangle, endangle);
    }

} // namespace IsovistUtils
