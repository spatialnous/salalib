// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pushvalues.h"

#include "attributetable.h"

void PushValues::pushValue(double &val, int &count, double thisval, Func pushFunc) {
    if (thisval != -1) {
        switch (pushFunc) {
        case Func::MAX:
            if (val == -1 || thisval > val)
                val = thisval;
            break;
        case Func::MIN:
            if (val == -1 || thisval < val)
                val = thisval;
            break;
        case Func::AVG:
        case Func::TOT:
            if (val == -1.0)
                val = thisval;
            else
                val += thisval;
            break;
        case Func::NONE:
            break;
        }
        count++;
    }
}

std::tuple<std::optional<size_t>, size_t, std::optional<size_t>>
PushValues::getColumnIndices(const AttributeTable &sourceAttr, std::optional<std::string> colIn,
                             AttributeTable &destAttr, std::string colOut,
                             std::optional<std::string> countCol) {

    std::optional<size_t> colInIdx = std::nullopt;
    if (colIn.has_value()) {
        colInIdx = sourceAttr.getColumnIndexOptional(colIn.value());
        if (!colInIdx.has_value()) {
            throw PushValueError("Column " + colIn.value() + " has not been found in source table");
        }
    };
    std::optional<size_t> colOutIdx = destAttr.getColumnIndexOptional(colOut);
    if (!colOutIdx.has_value()) {
        throw PushValueError("Column " + colOut + " has not been found in destination table");
    }
    std::optional<size_t> countColIdx = std::nullopt;
    if (countCol.has_value()) {
        countColIdx = destAttr.getColumnIndexOptional(countCol.value());
        if (!countColIdx.has_value()) {
            throw PushValueError("Column " + countCol.value() +
                                 " has not been found in destination table");
        }
    };
    return std::make_tuple(colInIdx, colOutIdx.value(), countColIdx);
}

std::tuple<size_t, size_t, std::optional<size_t>>
PushValues::getColumnIndices(const AttributeTable &sourceAttr, std::string colIn,
                             AttributeTable &destAttr, std::string colOut,
                             std::optional<std::string> countCol) {

    std::optional<size_t> colInIdx = sourceAttr.getColumnIndexOptional(colIn);
    if (!colInIdx.has_value()) {
        throw PushValueError("Column " + colIn + " has not been found in destination table");
    }
    std::optional<size_t> colOutIdx = destAttr.getColumnIndexOptional(colOut);
    if (!colOutIdx.has_value()) {
        throw PushValueError("Column " + colOut + " has not been found in destination table");
    }
    std::optional<size_t> countColIdx = std::nullopt;
    if (countCol.has_value()) {
        countColIdx = destAttr.getColumnIndexOptional(countCol.value());
        if (!countColIdx.has_value()) {
            throw PushValueError("Column " + countCol.value() +
                                 " has not been found in destination table");
        }
    };
    return std::make_tuple(colInIdx.value(), colOutIdx.value(), countColIdx);
}

void PushValues::shapeToPoint(const ShapeMap &sourceMap, std::string colIn, PointMap &destMap,
                              std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {
    auto &tableIn = sourceMap.getAttributeTable();
    auto &tableOut = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(tableIn, colIn, tableOut, colOut, countCol);

    // pushing from a shapemap (data/axial/segment/convex) requires a
    // combination of finding points (VGA) in polygons (convex and data maps
    // with polygons) and points that are on lines (axial, segment and data maps
    // with lines). Thus, in this case a composite approach is implemented,
    // which takes both options from the other parts of this conditional.

    struct ValueCountRow {
        double value = -1;
        int count = 0;
        AttributeRow &row;
        ValueCountRow(AttributeRow &row) : row(row) {}
    };

    // prepare a temporary value table to store counts and values
    std::map<AttributeKey, ValueCountRow> valCounts;

    for (auto &row : tableOut) {
        valCounts.insert(std::make_pair(row.getKey(),
                                        ValueCountRow(row.getRow()))); // count set to zero for all
    }

    // first collect the lines by pixelating them using the vga map
    auto &shapeMap = sourceMap.getAllShapes();
    for (auto &shape : shapeMap) {
        float thisval = tableIn.getRow(AttributeKey(shape.first)).getValue(colInIdx);
        if (shape.second.isLine()) {
            PixelRefVector linePixels = destMap.pixelateLine(shape.second.getLine());
            for (const PixelRef &pix : linePixels) {
                if (!destMap.getPoint(pix).filled())
                    continue;
                auto valCount = valCounts.find(AttributeKey(pix));
                if (valCount != valCounts.end()) {
                    pushValue(valCount->second.value, valCount->second.count, thisval, pushFunc);
                }
            }
        } else if (shape.second.isPolyLine()) {
            std::set<PixelRef> polylinePixels;
            for (size_t i = 1; i < shape.second.points.size(); i++) {
                Line li(shape.second.points[i - 1], shape.second.points[i]);
                PixelRefVector linePixels = destMap.pixelateLine(li);
                polylinePixels.insert(linePixels.begin(), linePixels.end());
            }
            for (const PixelRef &pix : polylinePixels) {
                if (!destMap.getPoint(pix).filled())
                    continue;
                auto valCount = valCounts.find(AttributeKey(pix));
                if (valCount != valCounts.end()) {
                    pushValue(valCount->second.value, valCount->second.count, thisval, pushFunc);
                }
            }
        }
    }

    // then collect the polygons and push to vga map
    for (auto &valCount : valCounts) {
        int keyOut = valCount.first.value;
        double &val = valCount.second.value;
        int &count = valCount.second.count;
        AttributeRow &row = valCount.second.row;
        std::vector<size_t> gatelist;
        if (!isObjectVisible(destMap.getLayers(), row)) {
            continue;
        }
        gatelist = sourceMap.pointInPolyList(destMap.getPoint(keyOut).getLocation());
        for (auto gate : gatelist) {
            auto &rowIn = sourceMap.getAttributeRowFromShapeIndex(gate);
            if (isObjectVisible(sourceMap.getLayers(), rowIn)) {
                double thisval = rowIn.getValue(colInIdx);
                pushValue(val, count, thisval, pushFunc);
            }
        }
        if (pushFunc == Func::AVG && val != -1.0) {
            val /= double(count);
        }
        row.setValue(colOutIdx, float(val));
        if (countColIdx.has_value()) {
            row.setValue(countColIdx.value(), float(count));
        }
    }
}

void PushValues::shapeToAxial(ShapeMap &sourceMap, std::optional<std::string> colIn,
                              ShapeGraph &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {

    auto &tableIn = sourceMap.getAttributeTable();
    auto &tableOut = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(tableIn, colIn, tableOut, colOut, countCol);

    for (auto iterOut = tableOut.begin(); iterOut != tableOut.end(); iterOut++) {
        int keyOut = iterOut->getKey().value;
        std::vector<size_t> gatelist;
        if (!isObjectVisible(destMap.getLayers(), iterOut->getRow())) {
            continue;
        }
        auto shapeMap = destMap.getAllShapes();
        gatelist = sourceMap.shapeInPolyList(shapeMap[keyOut]);

        double val = -1.0;
        int count = 0;
        for (auto gate : gatelist) {
            auto &rowIn = sourceMap.getAttributeRowFromShapeIndex(gate);

            if (isObjectVisible(sourceMap.getLayers(), rowIn)) {
                double thisval = static_cast<double>(gate);
                if (colInIdx.has_value())
                    thisval = rowIn.getValue(colInIdx.value());
                pushValue(val, count, thisval, pushFunc);
            }
        }
        if (pushFunc == Func::AVG && val != -1.0) {
            val /= double(count);
        }
        iterOut->getRow().setValue(colOutIdx, float(val));
        if (countColIdx.has_value()) {
            iterOut->getRow().setValue(countColIdx.value(), float(count));
        }
    }
}

void PushValues::shapeToShape(ShapeMap &sourceMap, std::optional<std::string> colIn,
                              ShapeMap &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {
    auto &tableIn = sourceMap.getAttributeTable();
    auto &tableOut = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(tableIn, colIn, tableOut, colOut, countCol);

    for (auto iterOut = tableOut.begin(); iterOut != tableOut.end(); iterOut++) {
        int keyOut = iterOut->getKey().value;
        std::vector<size_t> gatelist;

        if (!isObjectVisible(destMap.getLayers(), iterOut->getRow())) {
            continue;
        }
        auto dataMap = destMap.getAllShapes();
        gatelist = sourceMap.shapeInPolyList(dataMap[keyOut]);

        double val = -1.0;
        int count = 0;
        for (auto gate : gatelist) {
            auto &rowIn = sourceMap.getAttributeRowFromShapeIndex(gate);

            if (isObjectVisible(sourceMap.getLayers(), rowIn)) {
                double thisval = static_cast<double>(gate);
                if (colInIdx.has_value())
                    thisval = rowIn.getValue(colInIdx.value());
                pushValue(val, count, thisval, pushFunc);
            }
        }
        if (pushFunc == Func::AVG && val != -1.0) {
            val /= double(count);
        }
        iterOut->getRow().setValue(colOutIdx, float(val));
        if (countColIdx.has_value()) {
            iterOut->getRow().setValue(countColIdx.value(), float(count));
        }
    }
}

void PushValues::pointToShape(const PointMap &sourceMap, std::optional<std::string> colIn,
                              ShapeMap &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {

    auto &tableIn = sourceMap.getAttributeTable();
    auto &tableOut = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(tableIn, colIn, tableOut, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(tableOut.getNumRows());
    std::vector<int> counts(tableOut.getNumRows());

    for (size_t i = 0; i < tableOut.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }

    for (auto iterIn = tableIn.begin(); iterIn != tableIn.end(); iterIn++) {
        int pixIn = iterIn->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iterIn->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        gatelist = destMap.pointInPolyList(sourceMap.getPoint(pixIn).getLocation());
        double thisval = iterIn->getKey().value;
        if (colInIdx.has_value())
            thisval = iterIn->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            AttributeRow &rowOut = destMap.getAttributeRowFromShapeIndex(gate);
            if (isObjectVisible(destMap.getLayers(), rowOut)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = tableOut.begin(); iter != tableOut.end(); iter++) {

        if (!isObjectVisible(destMap.getLayers(), iter->getRow())) {
            i++;
            continue;
        }
        if (pushFunc == Func::AVG && vals[i] != -1.0) {
            vals[i] /= double(counts[i]);
        }
        iter->getRow().setValue(colOutIdx, float(vals[i]));
        if (countColIdx.has_value()) {
            iter->getRow().setValue(countColIdx.value(), float(counts[i]));
        }
        i++;
    }
}

void PushValues::pointToAxial(const PointMap &sourceMap, std::optional<std::string> colIn,
                              ShapeGraph &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {

    auto &tableIn = sourceMap.getAttributeTable();
    auto &tableOut = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(tableIn, colIn, tableOut, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(tableOut.getNumRows());
    std::vector<int> counts(tableOut.getNumRows());

    for (size_t i = 0; i < tableOut.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }

    for (auto iterIn = tableIn.begin(); iterIn != tableIn.end(); iterIn++) {
        int pixIn = iterIn->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iterIn->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        // note, "axial" could be convex map, and hence this would be a valid
        // operation
        gatelist = destMap.pointInPolyList(sourceMap.getPoint(pixIn).getLocation());
        double thisval = iterIn->getKey().value;
        if (colInIdx.has_value())
            thisval = iterIn->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            int keyOut = destMap.getShapeRefFromIndex(gate)->first;
            AttributeRow &rowOut = tableOut.getRow(AttributeKey(keyOut));
            if (isObjectVisible(destMap.getLayers(), rowOut)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = tableOut.begin(); iter != tableOut.end(); iter++) {

        if (!isObjectVisible(destMap.getLayers(), iter->getRow())) {
            i++;
            continue;
        }
        if (pushFunc == Func::AVG && vals[i] != -1.0) {
            vals[i] /= double(counts[i]);
        }
        iter->getRow().setValue(colOutIdx, float(vals[i]));
        if (countColIdx.has_value()) {
            iter->getRow().setValue(countColIdx.value(), float(counts[i]));
        }
        i++;
    }
}

void PushValues::axialToShape(const ShapeGraph &sourceMap, std::optional<std::string> colIn,
                              ShapeMap &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {

    auto &tableIn = sourceMap.getAttributeTable();
    auto &tableOut = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(tableIn, colIn, tableOut, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(tableOut.getNumRows());
    std::vector<int> counts(tableOut.getNumRows());

    for (size_t i = 0; i < tableOut.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }
    // note, in the spirit of mapping fewer objects in the gate list, it is
    // *usually* best to perform axial -> gate map in this direction
    for (auto iterIn = tableIn.begin(); iterIn != tableIn.end(); iterIn++) {
        int keyIn = iterIn->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iterIn->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        auto dataMap = sourceMap.getAllShapes();
        gatelist = destMap.shapeInPolyList(dataMap[keyIn]);
        double thisval = iterIn->getKey().value;
        if (colInIdx.has_value())
            thisval = iterIn->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            int keyOut = destMap.getShapeRefFromIndex(gate)->first;
            AttributeRow &rowOut = tableOut.getRow(AttributeKey(keyOut));
            if (isObjectVisible(destMap.getLayers(), rowOut)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = tableOut.begin(); iter != tableOut.end(); iter++) {

        if (!isObjectVisible(destMap.getLayers(), iter->getRow())) {
            i++;
            continue;
        }
        if (pushFunc == Func::AVG && vals[i] != -1.0) {
            vals[i] /= double(counts[i]);
        }
        iter->getRow().setValue(colOutIdx, float(vals[i]));
        if (countColIdx.has_value()) {
            iter->getRow().setValue(countColIdx.value(), float(counts[i]));
        }
        i++;
    }
}
void PushValues::axialToAxial(const ShapeGraph &sourceMap, std::optional<std::string> colIn,
                              ShapeGraph &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {

    auto &tableIn = sourceMap.getAttributeTable();
    auto &tableOut = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(tableIn, colIn, tableOut, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(tableOut.getNumRows());
    std::vector<int> counts(tableOut.getNumRows());

    for (size_t i = 0; i < tableOut.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }
    // note, in the spirit of mapping fewer objects in the gate list, it is
    // *usually* best to perform axial -> gate map in this direction
    for (auto iterIn = tableIn.begin(); iterIn != tableIn.end(); iterIn++) {
        int keyIn = iterIn->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iterIn->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        auto shapeMap = sourceMap.getAllShapes();
        gatelist = destMap.shapeInPolyList(shapeMap[keyIn]);
        double thisval = iterIn->getKey().value;
        if (colInIdx.has_value())
            thisval = iterIn->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            int keyOut = destMap.getShapeRefFromIndex(gate)->first;
            AttributeRow &rowOut = tableOut.getRow(AttributeKey(keyOut));
            if (isObjectVisible(destMap.getLayers(), rowOut)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = tableOut.begin(); iter != tableOut.end(); iter++) {

        if (!isObjectVisible(destMap.getLayers(), iter->getRow())) {
            i++;
            continue;
        }
        if (pushFunc == Func::AVG && vals[i] != -1.0) {
            vals[i] /= double(counts[i]);
        }
        iter->getRow().setValue(colOutIdx, float(vals[i]));
        if (countColIdx.has_value()) {
            iter->getRow().setValue(countColIdx.value(), float(counts[i]));
        }
        i++;
    }
}
