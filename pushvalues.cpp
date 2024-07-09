// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pushvalues.h"

#include "salalib/attributetable.h"

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
    auto &table_in = sourceMap.getAttributeTable();
    auto &table_out = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(table_in, colIn, table_out, colOut, countCol);

    // pushing from a shapemap (data/axial/segment/convex) requires a
    // combination of finding points (VGA) in polygons (convex and data maps
    // with polygons) and points that are on lines (axial, segment and data maps
    // with lines). Thus, in this case a composite approach is implemented,
    // which takes both options from the other parts of this conditional.

    struct ValueCountRow {
        double m_value = -1;
        int m_count = 0;
        AttributeRow &m_row;
        ValueCountRow(AttributeRow &row) : m_row(row) {}
    };

    // prepare a temporary value table to store counts and values
    std::map<AttributeKey, ValueCountRow> valCounts;

    for (auto &row : table_out) {
        valCounts.insert(std::make_pair(row.getKey(),
                                        ValueCountRow(row.getRow()))); // count set to zero for all
    }

    // first collect the lines by pixelating them using the vga map
    auto &shapeMap = sourceMap.getAllShapes();
    for (auto &shape : shapeMap) {
        float thisval = table_in.getRow(AttributeKey(shape.first)).getValue(colInIdx);
        if (shape.second.isLine()) {
            PixelRefVector linePixels = destMap.pixelateLine(shape.second.getLine());
            for (const PixelRef &pix : linePixels) {
                if (!destMap.getPoint(pix).filled())
                    continue;
                auto valCount = valCounts.find(AttributeKey(pix));
                if (valCount != valCounts.end()) {
                    pushValue(valCount->second.m_value, valCount->second.m_count, thisval,
                              pushFunc);
                }
            }
        } else if (shape.second.isPolyLine()) {
            std::set<PixelRef> polylinePixels;
            for (size_t i = 1; i < shape.second.m_points.size(); i++) {
                Line li(shape.second.m_points[i - 1], shape.second.m_points[i]);
                PixelRefVector linePixels = destMap.pixelateLine(li);
                polylinePixels.insert(linePixels.begin(), linePixels.end());
            }
            for (const PixelRef &pix : polylinePixels) {
                if (!destMap.getPoint(pix).filled())
                    continue;
                auto valCount = valCounts.find(AttributeKey(pix));
                if (valCount != valCounts.end()) {
                    pushValue(valCount->second.m_value, valCount->second.m_count, thisval,
                              pushFunc);
                }
            }
        }
    }

    // then collect the polygons and push to vga map
    for (auto &valCount : valCounts) {
        int key_out = valCount.first.value;
        double &val = valCount.second.m_value;
        int &count = valCount.second.m_count;
        AttributeRow &row = valCount.second.m_row;
        std::vector<size_t> gatelist;
        if (!isObjectVisible(destMap.getLayers(), row)) {
            continue;
        }
        gatelist = sourceMap.pointInPolyList(destMap.getPoint(key_out).getLocation());
        for (auto gate : gatelist) {
            auto &row_in = sourceMap.getAttributeRowFromShapeIndex(gate);
            if (isObjectVisible(sourceMap.getLayers(), row_in)) {
                double thisval = row_in.getValue(colInIdx);
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

    auto &table_in = sourceMap.getAttributeTable();
    auto &table_out = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(table_in, colIn, table_out, colOut, countCol);

    for (auto iter_out = table_out.begin(); iter_out != table_out.end(); iter_out++) {
        int key_out = iter_out->getKey().value;
        std::vector<size_t> gatelist;
        if (!isObjectVisible(destMap.getLayers(), iter_out->getRow())) {
            continue;
        }
        auto shapeMap = destMap.getAllShapes();
        gatelist = sourceMap.shapeInPolyList(shapeMap[key_out]);

        double val = -1.0;
        int count = 0;
        for (auto gate : gatelist) {
            auto &row_in = sourceMap.getAttributeRowFromShapeIndex(gate);

            if (isObjectVisible(sourceMap.getLayers(), row_in)) {
                double thisval = static_cast<double>(gate);
                if (colInIdx.has_value())
                    thisval = row_in.getValue(colInIdx.value());
                pushValue(val, count, thisval, pushFunc);
            }
        }
        if (pushFunc == Func::AVG && val != -1.0) {
            val /= double(count);
        }
        iter_out->getRow().setValue(colOutIdx, float(val));
        if (countColIdx.has_value()) {
            iter_out->getRow().setValue(countColIdx.value(), float(count));
        }
    }
}

void PushValues::shapeToShape(ShapeMap &sourceMap, std::optional<std::string> colIn,
                              ShapeMap &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {
    auto &table_in = sourceMap.getAttributeTable();
    auto &table_out = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(table_in, colIn, table_out, colOut, countCol);

    for (auto iter_out = table_out.begin(); iter_out != table_out.end(); iter_out++) {
        int key_out = iter_out->getKey().value;
        std::vector<size_t> gatelist;

        if (!isObjectVisible(destMap.getLayers(), iter_out->getRow())) {
            continue;
        }
        auto dataMap = destMap.getAllShapes();
        gatelist = sourceMap.shapeInPolyList(dataMap[key_out]);

        double val = -1.0;
        int count = 0;
        for (auto gate : gatelist) {
            auto &row_in = sourceMap.getAttributeRowFromShapeIndex(gate);

            if (isObjectVisible(sourceMap.getLayers(), row_in)) {
                double thisval = static_cast<double>(gate);
                if (colInIdx.has_value())
                    thisval = row_in.getValue(colInIdx.value());
                pushValue(val, count, thisval, pushFunc);
            }
        }
        if (pushFunc == Func::AVG && val != -1.0) {
            val /= double(count);
        }
        iter_out->getRow().setValue(colOutIdx, float(val));
        if (countColIdx.has_value()) {
            iter_out->getRow().setValue(countColIdx.value(), float(count));
        }
    }
}

void PushValues::pointToShape(const PointMap &sourceMap, std::optional<std::string> colIn,
                              ShapeMap &destMap, std::string colOut, Func pushFunc,
                              std::optional<std::string> countCol) {

    auto &table_in = sourceMap.getAttributeTable();
    auto &table_out = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(table_in, colIn, table_out, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(table_out.getNumRows());
    std::vector<int> counts(table_out.getNumRows());

    for (size_t i = 0; i < table_out.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }

    for (auto iter_in = table_in.begin(); iter_in != table_in.end(); iter_in++) {
        int pix_in = iter_in->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iter_in->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        gatelist = destMap.pointInPolyList(sourceMap.getPoint(pix_in).getLocation());
        double thisval = iter_in->getKey().value;
        if (colInIdx.has_value())
            thisval = iter_in->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            AttributeRow &row_out = destMap.getAttributeRowFromShapeIndex(gate);
            if (isObjectVisible(destMap.getLayers(), row_out)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = table_out.begin(); iter != table_out.end(); iter++) {

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

    auto &table_in = sourceMap.getAttributeTable();
    auto &table_out = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(table_in, colIn, table_out, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(table_out.getNumRows());
    std::vector<int> counts(table_out.getNumRows());

    for (size_t i = 0; i < table_out.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }

    for (auto iter_in = table_in.begin(); iter_in != table_in.end(); iter_in++) {
        int pix_in = iter_in->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iter_in->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        // note, "axial" could be convex map, and hence this would be a valid
        // operation
        gatelist = destMap.pointInPolyList(sourceMap.getPoint(pix_in).getLocation());
        double thisval = iter_in->getKey().value;
        if (colInIdx.has_value())
            thisval = iter_in->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            int key_out = destMap.getShapeRefFromIndex(gate)->first;
            AttributeRow &row_out = table_out.getRow(AttributeKey(key_out));
            if (isObjectVisible(destMap.getLayers(), row_out)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = table_out.begin(); iter != table_out.end(); iter++) {

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

    auto &table_in = sourceMap.getAttributeTable();
    auto &table_out = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(table_in, colIn, table_out, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(table_out.getNumRows());
    std::vector<int> counts(table_out.getNumRows());

    for (size_t i = 0; i < table_out.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }
    // note, in the spirit of mapping fewer objects in the gate list, it is
    // *usually* best to perform axial -> gate map in this direction
    for (auto iter_in = table_in.begin(); iter_in != table_in.end(); iter_in++) {
        int key_in = iter_in->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iter_in->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        auto dataMap = sourceMap.getAllShapes();
        gatelist = destMap.shapeInPolyList(dataMap[key_in]);
        double thisval = iter_in->getKey().value;
        if (colInIdx.has_value())
            thisval = iter_in->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            int key_out = destMap.getShapeRefFromIndex(gate)->first;
            AttributeRow &row_out = table_out.getRow(AttributeKey(key_out));
            if (isObjectVisible(destMap.getLayers(), row_out)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = table_out.begin(); iter != table_out.end(); iter++) {

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

    auto &table_in = sourceMap.getAttributeTable();
    auto &table_out = destMap.getAttributeTable();

    auto [colInIdx, colOutIdx, countColIdx] =
        getColumnIndices(table_in, colIn, table_out, colOut, countCol);

    // prepare a temporary value table to store counts and values
    std::vector<double> vals(table_out.getNumRows());
    std::vector<int> counts(table_out.getNumRows());

    for (size_t i = 0; i < table_out.getNumRows(); i++) {
        counts[i] = 0; // count set to zero for all
        vals[i] = -1;
    }
    // note, in the spirit of mapping fewer objects in the gate list, it is
    // *usually* best to perform axial -> gate map in this direction
    for (auto iter_in = table_in.begin(); iter_in != table_in.end(); iter_in++) {
        int key_in = iter_in->getKey().value;
        if (!isObjectVisible(sourceMap.getLayers(), iter_in->getRow())) {
            continue;
        }
        std::vector<size_t> gatelist;
        auto shapeMap = sourceMap.getAllShapes();
        gatelist = destMap.shapeInPolyList(shapeMap[key_in]);
        double thisval = iter_in->getKey().value;
        if (colInIdx.has_value())
            thisval = iter_in->getRow().getValue(colInIdx.value());
        for (auto gate : gatelist) {
            int key_out = destMap.getShapeRefFromIndex(gate)->first;
            AttributeRow &row_out = table_out.getRow(AttributeKey(key_out));
            if (isObjectVisible(destMap.getLayers(), row_out)) {
                double &val = vals[gate];
                int &count = counts[gate];
                pushValue(val, count, thisval, pushFunc);
            }
        }
    }
    size_t i = 0;
    for (auto iter = table_out.begin(); iter != table_out.end(); iter++) {

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
