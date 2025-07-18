// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017 Christian Sailer
// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mapconverter.hpp"

#include "tidylines.hpp"

#include "genlib/exceptions.hpp"
#include "genlib/stringutils.hpp"

#include <numeric>

// convert line layers to an axial map

std::unique_ptr<ShapeGraph> MapConverter::convertDrawingToAxial(
    Communicator *comm, const std::string &name,
    const std::vector<std::pair<std::reference_wrapper<const ShapeMap>, int>> &drawingMaps) {
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }

    Region4f region;
    std::map<int, std::pair<Line4f, int>>
        lines; // map required for tidy lines, otherwise acts like vector
    // this is used to say which layer it originated from

    bool recordlayer = false;

    // add all visible layers to the set of polygon lines...
    int count = 0;
    for (const auto &mapLayer : drawingMaps) {
        int j = mapLayer.second;
        if (region.atZero()) {
            region = mapLayer.first.get().getRegion();
        } else {
            region = region.runion(mapLayer.first.get().getRegion());
        }
        std::vector<SimpleLine> newLines = mapLayer.first.get().getAllShapesAsSimpleLines();
        for (const auto &line : newLines) {
            lines.insert(
                std::make_pair(count, std::make_pair(Line4f(line.start(), line.end()), j)));
            count++;
        }
        if (j > 0) {
            recordlayer = true;
        }
    }
    if (count == 0) {
        // TODO: write a better error message
        throw genlib::RuntimeException("Failed to convert lines");
    }

    // quick tidy removes very short and duplicate lines, but does not merge
    // overlapping lines
    TidyLines tidier;
    tidier.quicktidy(lines, region);
    if (lines.size() == 0) {
        throw genlib::RuntimeException("No lines found after removing short and duplicates");
    }

    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);
    }

    // create map layer...
    // we can stop here for all line axial map!
    std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name, ShapeMap::AXIALMAP));

    usermap->init(lines.size(), region); // used to be double density
    std::map<size_t, float> layerAttributes;
    usermap->initialiseAttributesAxial();
    std::optional<size_t> layerCol = std::nullopt;
    if (recordlayer) {
        layerCol =
            static_cast<int>(usermap->getAttributeTable().getOrInsertColumn("Drawing Layer"));
    }
    for (auto &line : lines) {
        if (recordlayer) {
            layerAttributes[layerCol.value()] = static_cast<float>(line.second.second);
        }
        usermap->makeLineShape(line.second.first, false, false, layerAttributes);
    }

    usermap->makeConnections();

    return usermap;
}

// create axial map directly from data maps
// note that actually should be able to merge this code with the line layers,
// now both use similar code

std::unique_ptr<ShapeGraph> MapConverter::convertDataToAxial(Communicator *comm,
                                                             const std::string &name,
                                                             ShapeMap &shapemap, bool copydata) {
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }

    // add all visible layers to the set of polygon lines...

    std::map<int, std::pair<Line4f, int>> lines;

    // m_region = shapemap.getRegion();
    Region4f region = shapemap.getRegion();

    // add all visible layers to the set of polygon lines...

    int count = 0;
    for (const auto &shape : shapemap.getAllShapes()) {
        int key = shape.first;

        std::vector<Line4f> shapeLines = shape.second.getAsLines();
        for (Line4f line : shapeLines) {
            lines.insert(std::make_pair(count, std::make_pair(line, key)));
            count++;
        }
    }
    if (lines.size() == 0) {
        throw genlib::RuntimeException("No lines found in data map");
    }

    // quick tidy removes very short and duplicate lines, but does not merge
    // overlapping lines
    TidyLines tidier;
    tidier.quicktidy(lines, region);
    if (lines.size() == 0) {
        throw genlib::RuntimeException("No lines found after removing short and duplicates");
    }

    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);
    }

    // create map layer...
    // we can stop here for all line axial map!
    std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name, ShapeMap::AXIALMAP));

    usermap->init(lines.size(), region); // used to be double density
    usermap->initialiseAttributesAxial();

    usermap->getAttributeTable().insertOrResetColumn("Data Map Ref");

    std::map<size_t, float> extraAttr;
    std::map<size_t, size_t> inOutColumns;
    if (copydata) {
        AttributeTable &input = shapemap.getAttributeTable();
        AttributeTable &output = usermap->getAttributeTable();

        // The columns used to be sorted in the old implementation, but are not
        // sorted anywhere else

        std::vector<size_t> indices(input.getNumColumns());
        std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

        std::vector<std::string> newColumns;
        for (size_t i = 0; i < indices.size(); i++) {
            auto idx = indices[i];
            std::string colname = input.getColumnName(idx);
            for (size_t k = 1; output.hasColumn(colname); k++) {
                colname =
                    dXstring::formatString(static_cast<int>(k), input.getColumnName(idx) + " %d");
            }
            newColumns.push_back(colname);
            output.insertOrResetColumn(colname);
        }
        for (size_t i = 0; i < indices.size(); i++) {
            inOutColumns[indices[i]] = output.getOrInsertColumn(newColumns[i]);
        }
    }

    auto dataMapShapeRefCol = usermap->getAttributeTable().getOrInsertColumn("Data Map Ref");

    AttributeTable &input = shapemap.getAttributeTable();
    for (auto &line : lines) {
        if (copydata) {
            auto &row = input.getRow(AttributeKey(line.second.second));
            for (auto inOutColumn : inOutColumns) {
                extraAttr[inOutColumn.second] =
                    row.getValue(static_cast<size_t>(inOutColumn.first));
            }
        }
        extraAttr[dataMapShapeRefCol] = static_cast<float>(line.second.second);
        usermap->makeLineShape(line.second.first, false, false, extraAttr);
    }

    // n.b. make connections also initialises attributes

    usermap->makeConnections();

    // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
    if (shapemap.hasMapInfoData()) {
        usermap->copyMapInfoBaseData(shapemap);
    }

    return usermap;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// yet more conversions, this time polygons to shape elements

std::unique_ptr<ShapeGraph> MapConverter::convertDrawingToConvex(
    Communicator *, const std::string &name,
    const std::vector<std::pair<std::reference_wrapper<const ShapeMap>, int>> &drawingMaps) {
    std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name, ShapeMap::CONVEXMAP));
    auto connCol =
        usermap->getAttributeTable().insertOrResetLockedColumn(ShapeGraph::Column::CONNECTIVITY);

    size_t count = 0;

    for (const auto &pixel : drawingMaps) {
        auto refShapes = pixel.first.get().getAllShapes();
        for (const auto &refShape : refShapes) {
            const SalaShape &shape = refShape.second;
            if (shape.isPolygon()) {
                int newShapeRef = usermap->makeShape(shape);
                usermap->getConnections().push_back(Connector());
                usermap->getAttributeTable().getRow(AttributeKey(newShapeRef)).setValue(connCol, 0);
                count++;
            }
        }
    }
    if (count == 0) {
        throw genlib::RuntimeException("No polygons found in drawing");
    }

    return usermap;
}

std::unique_ptr<ShapeGraph> MapConverter::convertDataToConvex(Communicator *,
                                                              const std::string &name,
                                                              ShapeMap &shapemap, bool copydata) {
    std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name, ShapeMap::CONVEXMAP));
    auto connCol =
        usermap->getAttributeTable().insertOrResetLockedColumn(ShapeGraph::Column::CONNECTIVITY);

    std::vector<int> lookup;
    auto refShapes = shapemap.getAllShapes();
    std::map<size_t, float> extraAttr;
    std::vector<size_t> attrCols;
    AttributeTable &input = shapemap.getAttributeTable();
    if (copydata) {
        AttributeTable &output = usermap->getAttributeTable();
        for (size_t i = 0; i < input.getNumColumns(); i++) {
            std::string colname = input.getColumnName(i);
            for (int k = 1; static_cast<int>(output.getColumnIndex(colname)) != -1; k++) {
                colname = dXstring::formatString(k, input.getColumnName(i) + " %d");
            }
            attrCols.push_back(output.insertOrResetColumn(colname));
        }
    }

    for (auto &refShape : refShapes) {
        if (copydata) {
            for (size_t i = 0; i < input.getNumColumns(); ++i) {
                extraAttr[attrCols[static_cast<size_t>(i)]] =
                    input.getRow(AttributeKey(refShape.first)).getValue(i);
            }
        }
        SalaShape &shape = refShape.second;
        if (shape.isPolygon()) {
            int n = usermap->makeShape(shape, -1, extraAttr);
            usermap->getConnections().push_back(Connector());
            usermap->getAttributeTable().getRow(AttributeKey(n)).setValue(connCol, 0);
        }
    }
    if (usermap->getShapeCount() == 0) {
        throw genlib::RuntimeException("No polygons found in data map");
    }

    return usermap;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

// create segment map directly from line layers

std::unique_ptr<ShapeGraph> MapConverter::convertDrawingToSegment(
    Communicator *comm, const std::string &name,
    const std::vector<std::pair<std::reference_wrapper<const ShapeMap>, int>> &drawingMaps) {
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }

    // second number in internal pair is used to say which layer it originated
    // from
    std::map<int, std::pair<Line4f, int>> lines;

    bool recordlayer = false;

    Region4f region;

    // add all visible layers to the set of polygon lines...
    int count = 0;
    for (const auto &mapLayer : drawingMaps) {
        int j = mapLayer.second;
        if (region.atZero()) {
            region = mapLayer.first.get().getRegion();
        } else {
            region = region.runion(mapLayer.first.get().getRegion());
        }
        std::vector<SimpleLine> newLines = mapLayer.first.get().getAllShapesAsSimpleLines();
        for (const auto &line : newLines) {
            lines.insert(
                std::make_pair(count, std::make_pair(Line4f(line.start(), line.end()), j)));
            count++;
        }
        if (j > 0) {
            recordlayer = true;
        }
    }
    if (count == 0) {
        throw genlib::RuntimeException("No lines found in drawing");
    }

    // quick tidy removes very short and duplicate lines, but does not merge
    // overlapping lines
    TidyLines tidier;
    tidier.quicktidy(lines, region);
    if (lines.size() == 0) {
        throw genlib::RuntimeException("No lines found after removing short and duplicates");
    }

    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);
    }

    // create map layer...
    // we can stop here for all line axial map!
    std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name, ShapeMap::SEGMENTMAP));

    usermap->init(lines.size(), region);
    std::map<size_t, float> layerAttributes;
    usermap->initialiseAttributesSegment();
    std::optional<size_t> layerCol = std::nullopt;
    if (recordlayer) {
        layerCol = usermap->getAttributeTable().insertOrResetColumn("Drawing Layer");
    }
    for (auto &line : lines) {
        if (recordlayer) {
            layerAttributes[layerCol.value()] = static_cast<float>(line.second.second);
        }
        usermap->makeLineShape(line.second.first, false, false, layerAttributes);
    }

    // make it!
    usermap->makeNewSegMap(comm);

    return usermap;
}

// create segment map directly from data maps (ultimately, this will replace the
// line layers version)

std::unique_ptr<ShapeGraph> MapConverter::convertDataToSegment(Communicator *comm,
                                                               const std::string &name,
                                                               ShapeMap &shapemap, bool copydata) {
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }

    std::map<int, std::pair<Line4f, int>> lines;

    // no longer requires m_region
    // m_region = shapemap.getRegion();
    Region4f region = shapemap.getRegion();

    // add all visible layers to the set of polygon lines...

    int count = 0;
    for (const auto &shape : shapemap.getAllShapes()) {
        int key = shape.first;
        std::vector<Line4f> shapeLines = shape.second.getAsLines();
        for (Line4f line : shapeLines) {
            lines.insert(std::make_pair(count, std::make_pair(line, key)));
            count++;
        }
    }
    if (lines.size() == 0) {
        throw genlib::RuntimeException("No lines found in data map");
    }

    // quick tidy removes very short and duplicate lines, but does not merge
    // overlapping lines
    TidyLines tidier;
    tidier.quicktidy(lines, region);

    if (lines.size() == 0) {
        throw genlib::RuntimeException("No lines found after removing short and duplicates");
    }

    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);
    }

    // create map layer...
    // note, I may need to reuse this:
    std::unique_ptr<ShapeGraph> usermap(new ShapeGraph(name, ShapeMap::SEGMENTMAP));

    // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
    if (shapemap.hasMapInfoData()) {
        usermap->copyMapInfoBaseData(shapemap);
    }

    usermap->init(lines.size(), region);
    usermap->initialiseAttributesSegment();

    usermap->getAttributeTable().insertOrResetColumn("Data Map Ref");

    std::map<size_t, float> extraAttr;
    std::map<size_t, size_t> inOutColumns;
    if (copydata) {
        AttributeTable &input = shapemap.getAttributeTable();
        AttributeTable &output = usermap->getAttributeTable();

        // TODO: Compatibility. The columns are sorted in the old implementation so
        // they are also passed sorted in the conversion:

        std::vector<size_t> indices(input.getNumColumns());
        std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return input.getColumnName(a) < input.getColumnName(b);
        });

        std::vector<std::string> newColumns;
        for (size_t i = 0; i < indices.size(); i++) {
            auto idx = indices[i];
            std::string colname = input.getColumnName(idx);
            for (size_t k = 1; output.hasColumn(colname); k++) {
                colname =
                    dXstring::formatString(static_cast<int>(k), input.getColumnName(idx) + " %d");
            }
            newColumns.push_back(colname);
            output.insertOrResetColumn(colname);
        }
        for (size_t i = 0; i < indices.size(); i++) {
            inOutColumns[indices[i]] = output.getOrInsertColumn(newColumns[i]);
        }
    }

    auto dataMapShapeRefCol = usermap->getAttributeTable().getOrInsertColumn("Data Map Ref");

    AttributeTable &input = shapemap.getAttributeTable();
    for (auto &line : lines) {
        if (copydata) {
            auto &row = input.getRow(AttributeKey(line.second.second));
            for (auto inOutColumn : inOutColumns) {
                extraAttr[inOutColumn.second] = row.getValue(inOutColumn.first);
            }
        }
        extraAttr[dataMapShapeRefCol] = static_cast<float>(line.second.second);
        usermap->makeLineShape(line.second.first, false, false, extraAttr);
    }

    // start to be a little bit more efficient about memory now we are hitting the
    // limits from time to time:
    if (!copydata) {
        lines.clear();
    }

    // make it!
    usermap->makeNewSegMap(comm);

    return usermap;
}

// stubremoval is fraction of overhanging line length before axial "stub" is
// removed
std::unique_ptr<ShapeGraph>
MapConverter::convertAxialToSegment(Communicator *, ShapeGraph &axialMap, const std::string &name,
                                    bool keeporiginal, bool copydata, double stubremoval) {
    std::vector<Line4f> lines;
    std::vector<Connector> connectionset;

    axialMap.makeSegmentMap(lines, connectionset, stubremoval);

    // destroy unnecessary parts of axial map as quickly as possible in order not
    // to overload memory
    if (!keeporiginal) {
        axialMap.getAllShapes().clear();
        axialMap.getConnections().clear();
    }

    // create map layer...
    std::unique_ptr<ShapeGraph> segmap(new ShapeGraph(name, ShapeMap::SEGMENTMAP));

    segmap->init(lines.size(), axialMap.getRegion());
    segmap->initialiseAttributesSegment();

    for (size_t k = 0; k < lines.size(); k++) {
        segmap->makeLineShape(lines[k]);
    }

    // clear data as soon as we do not need it:
    lines.clear();

    // if we are inheriting from a mapinfo map, pass on the coordsys and bounds:
    if (axialMap.hasMapInfoData()) {
        segmap->copyMapInfoBaseData(axialMap);
    }

    segmap->makeSegmentConnections(connectionset);

    if (copydata) {
        segmap->pushAxialValues(axialMap);
    }
    // destroy unnecessary parts of axial map as quickly as possible in order not
    // to overload memory
    if (!keeporiginal) {
        axialMap.getAttributeTable().clear();
    }

    return segmap;
}
