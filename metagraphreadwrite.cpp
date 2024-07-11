﻿// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "metagraphreadwrite.h"

#include "alllinemap.h"
#include "mgraph_consts.h"
#include "shapemapgroupdata.h"

#include "genlib/readwritehelpers.h"
#include "genlib/stringutils.h"

#include <fstream>

namespace {
    // old depthmapX display information, left here to allow reading
    // metagraph files
    enum {
        NONE = 0x0000,
        POINTMAPS = 0x0002,
        LINEDATA = 0x0004,
        ANGULARGRAPH = 0x0010,
        DATAMAPS = 0x0020,
        AXIALLINES = 0x0040,
        SHAPEGRAPHS = 0x0100,
        BUGGY = 0x8000
    };
    enum {
        SHOWHIDEVGA = 0x0100,
        SHOWVGATOP = 0x0200,
        SHOWHIDEAXIAL = 0x0400,
        SHOWAXIALTOP = 0x0800,
        SHOWHIDESHAPE = 0x1000,
        SHOWSHAPETOP = 0x2000
    };
    enum {
        VIEWNONE = 0x00,
        VIEWVGA = 0x01,
        VIEWBACKVGA = 0x02,
        VIEWAXIAL = 0x04,
        VIEWBACKAXIAL = 0x08,
        VIEWDATA = 0x20,
        VIEWBACKDATA = 0x40,
        VIEWFRONT = 0x25
    };

    // These allow for the functions below to accept both maps (PointMap, Shapemap etc)
    // and std::reference_wrappers of the maps.
    template <typename T> struct is_reference_wrapper : std::false_type {};
    template <typename T>
    struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type {};

    template <typename T> const T &getMapRef(const T &&t, std::false_type) { return t; }
    template <typename T>
    const T &getMapRef(const std::reference_wrapper<T> &&ref, std::true_type) {
        return ref.get();
    }

} // namespace

MetaGraphReadWrite::MetaGraphData MetaGraphReadWrite::readFromFile(const std::string &filename) {

    if (filename.empty()) {
        throw MetaGraphReadError("File is not a MetaGraph");
    }

#ifdef _WIN32
    std::ifstream stream(filename.c_str(), std::ios::binary | std::ios::in);
#else
    std::ifstream stream(filename.c_str(), std::ios::in);
#endif
    auto result = MetaGraphReadWrite::readFromStream(stream);
    stream.close();
    return result;
}

void MetaGraphReadWrite::readHeader(std::istream &stream) {
    char header[3];
    stream.read(header, 3);
    if (stream.fail() || header[0] != 'g' || header[1] != 'r' || header[2] != 'f') {
        throw MetaGraphReadError("Could not read file, not a MetaGraph?");
    }
}

QtRegion MetaGraphReadWrite::readRegion(std::istream &stream) {
    QtRegion region;
    stream.read((char *)&region, sizeof(region));
    return region;
}

std::tuple<std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>>,
           std::vector<std::vector<std::tuple<bool, bool, int>>>>
MetaGraphReadWrite::readDrawingFiles(std::istream &stream) {
    int count;
    stream.read((char *)&count, sizeof(count));
    //    std::vector<std::pair<ShapeMapGroupData, ShapeMap>> ada;

    std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>> drawingFiles(count);
    std::vector<std::vector<std::tuple<bool, bool, int>>> displayData(count);
    for (int i = 0; i < count; i++) {
        drawingFiles[i].first.readInName(stream);

        std::tie(drawingFiles[i].second, displayData[i]) =
            ShapeMapGroupData::readSpacePixels(stream);
    }

    return std::make_tuple(std::move(drawingFiles), std::move(displayData));
}

MetaGraphReadWrite::MetaGraphData MetaGraphReadWrite::readFromStream(std::istream &stream) {

    readHeader(stream);

    MetaGraphData mgd;

    stream.read((char *)&mgd.version, sizeof(mgd.version));
    if (mgd.version > METAGRAPH_VERSION) {
        mgd.readStatus = ReadStatus::NEWER_VERSION;
        return mgd;
    }
    if (mgd.version < METAGRAPH_VERSION) {
        mgd.readStatus = ReadStatus::DEPRECATED_VERSION;
        return mgd;
    }

    // have to use temporary state here as redraw attempt may come too early:
    int tempState = 0;
    stream.read((char *)&tempState, sizeof(tempState));
    stream.read((char *)&mgd.displayData.viewClass, sizeof(mgd.displayData.viewClass));
    stream.read((char *)&mgd.displayData.showGrid, sizeof(mgd.displayData.showGrid));
    stream.read((char *)&mgd.displayData.showText, sizeof(mgd.displayData.showText));

    // type codes: x --- properties
    //             v --- virtual graph (from versions below 70)
    //             n --- ngraph format
    //             l --- layer data
    //             p --- point data
    //             d --- data summary layers

    char type;
    stream.read(&type, 1);
    if (type == 'd') {
        // contains deprecated datalayers. depthmapX should be able to
        // convert them into shapemaps
        mgd.readStatus = ReadStatus::DEPRECATED_VERSION;
        return mgd;
    }
    if (type == 'x') {
        mgd.metaGraph.fileProperties.read(stream);
        if (stream.eof()) {
            // erk... this shouldn't happen
            mgd.readStatus = ReadStatus::DAMAGED_FILE;
            return mgd;
        } else if (!stream.eof()) {
            stream.read(&type, 1);
        }
    } else {
        mgd.metaGraph.fileProperties.setProperties("<unknown>", "<unknown>", "<unknown>",
                                                   "<unknown>");
    }
    if (stream.eof()) {
        // file is still ok, just empty
        return mgd;
    }
    if (type == 'v') {

        skipVirtualMem(stream);

        if (stream.eof()) {
            // erk... this shouldn't happen
            mgd.readStatus = ReadStatus::DAMAGED_FILE;
            return mgd;
        } else if (!stream.eof()) {
            stream.read(&type, 1);
        }
    }
    if (type == 'l') {
        mgd.metaGraph.name = dXstring::readString(stream);
        if (mgd.metaGraph.name.empty()) {
            mgd.metaGraph.name = "<unknown>";
        }
        mgd.metaGraph.region = readRegion(stream);
        std::tie(mgd.drawingFiles, mgd.displayData.perDrawingMap) = readDrawingFiles(stream);
        tempState |= LINEDATA;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
        if (!stream.eof() && !stream.good()) {
            // erk... this shouldn't happen
            mgd.readStatus = ReadStatus::DAMAGED_FILE;
            return mgd;
        }
    }
    if (type == 'p') {
        std::tie(mgd.pointMaps, mgd.displayData.perPointMap, mgd.displayData.displayedPointMap) =
            MetaGraphReadWrite::readPointMaps(stream, mgd.metaGraph.region);
        tempState |= POINTMAPS;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
        if (type == 'g') {
            // record on state of actual point map:
            mgd.displayData.displayedPointMapProcessed = true;

            if (!stream.eof()) {
                stream.read(&type, 1);
            }
        }
    }
    if (type == 'a') {
        tempState |= ANGULARGRAPH;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
    }
    if (type == 'x') {
        std::tie(mgd.shapeGraphs, mgd.allLineMapIdx, mgd.displayData.perShapeGraph,
                 mgd.displayData.displayedShapeGraph) = MetaGraphReadWrite::readShapeGraphs(stream);
        tempState |= SHAPEGRAPHS;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
    }
    if (type == 's') {
        std::tie(mgd.dataMaps, mgd.displayData.perDataMap, mgd.displayData.displayedDataMap) =
            readDataMaps(stream);
        tempState |= DATAMAPS;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
    }
    mgd.displayData.state = tempState;

    mgd.readStatus = ReadStatus::OK;
    return mgd;
}

int MetaGraphReadWrite::write(const std::string &filename, const MetaGraphData &mgd) {
    auto &dd = mgd.displayData;
    return MetaGraphReadWrite::write(
        filename,
        // MetaGraph Data
        mgd.version, mgd.metaGraph.name, mgd.metaGraph.region, mgd.metaGraph.fileProperties,
        mgd.drawingFiles, mgd.pointMaps, mgd.dataMaps, mgd.shapeGraphs, mgd.allLineMapIdx,
        // display data
        dd.state, dd.viewClass, dd.showGrid, dd.showText, dd.perDrawingMap, dd.displayedPointMap,
        dd.perPointMap, dd.displayedDataMap, dd.perDataMap, dd.displayedShapeGraph,
        dd.perShapeGraph);
}

std::streampos MetaGraphReadWrite::skipVirtualMem(std::istream &stream) {
    // it's graph virtual memory: skip it
    int nodes = -1;
    stream.read((char *)&nodes, sizeof(nodes));

    nodes *= 2;

    for (int i = 0; i < nodes; i++) {
        int connections;
        stream.read((char *)&connections, sizeof(connections));
        stream.seekg(stream.tellg() + std::streamoff(connections * sizeof(connections)));
    }
    return (stream.tellg());
}

std::tuple<std::vector<PointMap>, std::vector<int>, unsigned int>
MetaGraphReadWrite::readPointMaps(std::istream &stream, QtRegion defaultRegion) {
    unsigned int displayedMap;
    stream.read((char *)&displayedMap, sizeof(displayedMap));
    std::vector<PointMap> pointMaps;
    std::vector<int> displayAttributes;
    int count;
    stream.read((char *)&count, sizeof(count));
    for (int i = 0; i < count; i++) {
        pointMaps.push_back(PointMap(defaultRegion));
        auto [completed, displayedAttribute] = pointMaps.back().read(stream);
        displayAttributes.push_back(displayedAttribute);
    }
    return std::make_tuple(std::move(pointMaps), std::move(displayAttributes), displayedMap);
}

template <typename PointMapOrRef>
bool MetaGraphReadWrite::writePointMaps(std::ofstream &stream,
                                        const std::vector<PointMapOrRef> &pointMaps,
                                        const std::vector<int> displayData,
                                        const unsigned int displayedMap) {
    unsigned int displayed_pointmap = displayedMap;
    stream.write((char *)&displayed_pointmap, sizeof(displayed_pointmap));
    auto count = pointMaps.size();
    stream.write((char *)&count, sizeof(static_cast<int>(count)));
    auto it = displayData.begin();
    for (auto &pointMap : pointMaps) {
        getMapRef(std::forward<const PointMapOrRef>(pointMap),
                  is_reference_wrapper<std::decay_t<const PointMapOrRef>>{})
            .write(stream, *it);
        it++;
    }
    return true;
}

std::tuple<std::vector<ShapeMap>, std::vector<std::tuple<bool, bool, int>>, unsigned int>
MetaGraphReadWrite::readDataMaps(std::istream &stream) {
    std::vector<ShapeMap> dataMaps;
    std::vector<std::tuple<bool, bool, int>> displayData;
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int displayedMap;
    stream.read((char *)&displayedMap, sizeof(displayedMap));
    // read maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int count = 0;
    stream.read((char *)&count, sizeof(count));

    for (size_t j = 0; j < size_t(count); j++) {
        dataMaps.emplace_back();
        auto [completed, editable, show, displayedAttribute] = dataMaps.back().read(stream);
        displayData.emplace_back(editable, show, displayedAttribute);
    }
    return std::make_tuple(std::move(dataMaps), std::move(displayData), displayedMap);
}

template <typename ShapeMapOrRef>
bool MetaGraphReadWrite::writeDataMaps(std::ofstream &stream,
                                       const std::vector<ShapeMapOrRef> &dataMaps,
                                       const std::vector<ShapeMapDisplayData> displayData,
                                       const unsigned int displayedMap) {
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    stream.write((char *)&displayedMap, sizeof(displayedMap));
    // write maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int count = (unsigned int)dataMaps.size();
    stream.write((char *)&count, sizeof(count));
    auto it = displayData.begin();
    for (auto &dataMap : dataMaps) {
        getMapRef(std::forward<const ShapeMapOrRef>(dataMap),
                  is_reference_wrapper<std::decay_t<const ShapeMapOrRef>>{})
            .write(stream, *it);
        it++;
    }
    return true;
}

template <typename ShapeMapOrRef>
bool MetaGraphReadWrite::writeSpacePixels(
    std::ostream &stream, const std::vector<ShapeMapOrRef> &spacePixels,
    const std::vector<std::tuple<bool, bool, int>> displayData) {

    int count = spacePixels.size();
    stream.write((char *)&count, sizeof(count));
    auto it = displayData.begin();
    for (auto &spacePixel : spacePixels) {

        getMapRef(std::forward<const ShapeMapOrRef>(spacePixel),
                  is_reference_wrapper<std::decay_t<const ShapeMapOrRef>>{})
            .write(stream, *it);
        it++;
    }
    return true;
}

std::tuple<std::vector<std::unique_ptr<ShapeGraph>>, std::optional<size_t>,
           std::vector<MetaGraphReadWrite::ShapeMapDisplayData>, unsigned int>
MetaGraphReadWrite::readShapeGraphs(std::istream &stream) {
    std::vector<std::unique_ptr<ShapeGraph>> shapeGraphs;
    std::vector<std::tuple<bool, bool, int>> displayData;
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int displayedMap;
    stream.read((char *)&displayedMap, sizeof(displayedMap));
    // read maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int count = 0;
    stream.read((char *)&count, sizeof(count));

    for (size_t j = 0; j < size_t(count); j++) {
        shapeGraphs.push_back(std::unique_ptr<ShapeGraph>(new ShapeGraph()));

        // P.K. Hairy solution given that we don't know the type/name of the
        // shapegraph before we actually read it: mark the beginning of the
        // shapegraph in the stream and if it's found to be an AllLineMap then just
        // roll the stream back and read from the mark again

        long mark = stream.tellg();
        auto [completed, editable, show, displayedAttribute] = shapeGraphs.back()->read(stream);
        std::string name = shapeGraphs.back()->getName();

        if (name == "All-Line Map" || name == "All Line Map") {
            shapeGraphs.pop_back();
            shapeGraphs.push_back(std::unique_ptr<AllLineMap>(new AllLineMap()));
            stream.seekg(mark);
            std::tie(completed, editable, show, displayedAttribute) =
                shapeGraphs.back()->read(stream);
        }
        displayData.emplace_back(editable, show, displayedAttribute);
    }

    // P.K: ideally this should be read together with the
    // all-line map, but the way the graph file is structured
    // this is not possible
    // TODO: Fix on next graph file update

    std::optional<size_t> allLineMapIdx = std::nullopt;
    for (size_t i = 0; i < shapeGraphs.size(); i++) {
        ShapeGraph *shapeGraph = shapeGraphs[i].get();
        if (shapeGraph->getName() == "All-Line Map" || shapeGraph->getName() == "All Line Map") {
            AllLineMap *alllinemap = dynamic_cast<AllLineMap *>(shapeGraph);
            if (alllinemap == nullptr) {
                throw MetaGraphReadError("Failed to cast from ShapeGraph to AllLineMap");
            }
            // these are additional essentially for all line axial maps
            // should probably be kept *with* the all line axial map...
            alllinemap->m_poly_connections.clear();
            dXreadwrite::readIntoVector(stream, alllinemap->m_poly_connections);
            alllinemap->m_radial_lines.clear();
            dXreadwrite::readIntoVector(stream, alllinemap->m_radial_lines);

            // this is an index to look up the all line map, used by UI to determine
            // if can make fewest line map note: it is not saved for historical
            // reasons will get confused by more than one all line map
            allLineMapIdx = i;

            // there is currently only one:
            break;
        }
    }
    if (!allLineMapIdx.has_value()) {
        // P.K. This is just a dummy read to cover cases where there is no All-Line
        // Map The below is taken from pmemvec<T>::read

        // READ / WRITE USES 32-bit LENGTHS (number of elements)
        // n.b., do not change this to size_t as it will cause 32-bit to 64-bit
        // conversion problems
        unsigned int length;
        stream.read((char *)&length, sizeof(unsigned int));
        stream.read((char *)&length, sizeof(unsigned int));
    }
    return std::make_tuple(std::move(shapeGraphs), allLineMapIdx, std::move(displayData),
                           displayedMap);
}

template <typename ShapeGraphOrRef>
bool MetaGraphReadWrite::writeShapeGraphs(
    std::ofstream &stream, const std::vector<ShapeGraphOrRef> &shapeGraphs,
    const std::optional<size_t> allLineMapIdx,
    const std::vector<std::tuple<bool, bool, int>> displayData,
    const unsigned int displayedShapeGraph) {
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    stream.write((char *)&displayedShapeGraph, sizeof(displayedShapeGraph));
    // write maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int count = (unsigned int)shapeGraphs.size();
    stream.write((char *)&count, sizeof(count));
    auto it = displayData.begin();
    for (auto &shapeGraphPtr : shapeGraphs) {
        getMapRef(std::forward<const ShapeGraphOrRef>(shapeGraphPtr),
                  is_reference_wrapper<std::decay_t<const ShapeGraphOrRef>>{})
            ->write(stream, *it);
        it++;
    }

    if (!allLineMapIdx.has_value()) {
        std::vector<PolyConnector> temp_poly_connections;
        std::vector<RadialLine> temp_radial_lines;

        dXreadwrite::writeVector(stream, temp_poly_connections);
        dXreadwrite::writeVector(stream, temp_radial_lines);
    } else {
        auto &shapeGraph =
            getMapRef(std::forward<const ShapeGraphOrRef>(shapeGraphs[allLineMapIdx.value()]),
                      is_reference_wrapper<std::decay_t<const ShapeGraphOrRef>>{});
        AllLineMap *alllinemap = dynamic_cast<AllLineMap *>(shapeGraph.get());

        if (alllinemap == nullptr) {
            throw MetaGraphReadError("Failed to cast from ShapeGraph to AllLineMap");
        }

        dXreadwrite::writeVector(stream, alllinemap->m_poly_connections);
        dXreadwrite::writeVector(stream, alllinemap->m_radial_lines);
    }
    return true;
}

template <typename PointMapOrRef, typename ShapeMapOrRef, typename ShapeGraphOrRef>
int MetaGraphReadWrite::write(
    const std::string &filename,
    // MetaGraph Data
    const int version, const std::string &name, const QtRegion &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMapOrRef>>> &drawingFiles,
    const std::vector<PointMapOrRef> &pointMaps, const std::vector<ShapeMapOrRef> &dataMaps,
    const std::vector<ShapeGraphOrRef> &shapeGraphs, const std::optional<size_t> allLineMapIdx,
    // display data
    const int state, const int viewClass, const bool showGrid, const bool showText,
    const std::vector<std::vector<ShapeMapDisplayData>> perDrawingMap,
    const unsigned int displayedPointMap, const std::vector<int> perPointMap,
    const unsigned int displayedDataMap, const std::vector<ShapeMapDisplayData> perDataMap,
    const unsigned int displayedShapeGraph, const std::vector<ShapeMapDisplayData> perShapeGraph) {

    enum {
        OK,
        WARN_BUGGY_VERSION,
        WARN_CONVERTED,
        NOT_A_GRAPH,
        DAMAGED_FILE,
        DISK_ERROR,
        NEWER_VERSION,
        DEPRECATED_VERSION
    };
    enum {
        NONE = 0x0000,
        POINTMAPS = 0x0002,
        LINEDATA = 0x0004,
        ANGULARGRAPH = 0x0010,
        DATAMAPS = 0x0020,
        AXIALLINES = 0x0040,
        SHAPEGRAPHS = 0x0100,
        BUGGY = 0x8000
    };

    std::ofstream stream;

    char type;

    // As of MetaGraph version 70 the disk caching has been removed
    stream.open(filename.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
    if (stream.fail()) {
        if (stream.rdbuf()->is_open()) {
            stream.close();
        }
        return DISK_ERROR;
    }
    stream.write("grf", 3);
    stream.write((char *)&version, sizeof(version));

    stream.write((char *)&state, sizeof(state));
    stream.write((char *)&viewClass, sizeof(viewClass));

    stream.write((char *)&showGrid, sizeof(showGrid));
    stream.write((char *)&showText, sizeof(showText));

    type = 'x';
    stream.write(&type, 1);
    fileProperties.write(stream);

    if (state & LINEDATA) {
        type = 'l';
        stream.write(&type, 1);
        dXstring::writeString(stream, name);
        stream.write((char *)&region, sizeof(region));

        int count = static_cast<int>(drawingFiles.size());
        stream.write((char *)&count, sizeof(count));
        auto it = perDrawingMap.begin();
        for (auto &spacePixel : drawingFiles) {
            spacePixel.first.writeOutName(stream);
            writeSpacePixels(stream, spacePixel.second, *it);
            it++;
        }
    }
    if (state & POINTMAPS) {
        type = 'p';
        stream.write(&type, 1);
        writePointMaps(stream, pointMaps, perPointMap, displayedPointMap);
    }
    if (state & SHAPEGRAPHS) {
        type = 'x';
        stream.write(&type, 1);
        writeShapeGraphs(stream, shapeGraphs, allLineMapIdx, perShapeGraph, displayedShapeGraph);
    }
    if (state & DATAMAPS) {
        type = 's';
        stream.write(&type, 1);
        writeDataMaps(stream, dataMaps, perDataMap, displayedDataMap);
    }

    stream.close();
    return OK;
}

// these are just explicit instantiations of the write function to allow the linker
// to find them when required
template int MetaGraphReadWrite::write<PointMap, ShapeMap, std::unique_ptr<ShapeGraph>>(
    const std::string &filename,
    // MetaGraph Data
    const int version, const std::string &name, const QtRegion &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>> &drawingFiles,
    const std::vector<PointMap> &pointMaps, const std::vector<ShapeMap> &dataMaps,
    const std::vector<std::unique_ptr<ShapeGraph>> &shapeGraphs,
    const std::optional<size_t> allLineMapIdx,
    // display data
    const int state = 0, const int viewClass = 0, const bool showGrid = true,
    const bool showText = true,
    const std::vector<std::vector<ShapeMapDisplayData>> perDrawingMap =
        std::vector<std::vector<ShapeMapDisplayData>>(),
    const unsigned int displayedPointMap = 0,
    const std::vector<int> perPointMap = std::vector<int>(),
    const unsigned int displayedDataMap = 0,
    const std::vector<ShapeMapDisplayData> perDataMap = std::vector<ShapeMapDisplayData>(),
    const unsigned int displayedShapeGraph = 0,
    const std::vector<ShapeMapDisplayData> perShapeGraph = std::vector<ShapeMapDisplayData>());

template int
MetaGraphReadWrite::write<std::reference_wrapper<PointMap>, std::reference_wrapper<ShapeMap>,
                          std::reference_wrapper<std::unique_ptr<ShapeGraph>>>(
    const std::string &filename,
    // MetaGraph Data
    const int version, const std::string &name, const QtRegion &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<std::reference_wrapper<ShapeMap>>>>
        &drawingFiles,
    const std::vector<std::reference_wrapper<PointMap>> &pointMaps,
    const std::vector<std::reference_wrapper<ShapeMap>> &dataMaps,
    const std::vector<std::reference_wrapper<std::unique_ptr<ShapeGraph>>> &shapeGraphs,
    const std::optional<size_t> allLineMapIdx,
    // display data
    const int state = 0, const int viewClass = 0, const bool showGrid = true,
    const bool showText = true,
    const std::vector<std::vector<ShapeMapDisplayData>> perDrawingMap =
        std::vector<std::vector<ShapeMapDisplayData>>(),
    const unsigned int displayedPointMap = 0,
    const std::vector<int> perPointMap = std::vector<int>(),
    const unsigned int displayedDataMap = 0,
    const std::vector<ShapeMapDisplayData> perDataMap = std::vector<ShapeMapDisplayData>(),
    const unsigned int displayedShapeGraph = 0,
    const std::vector<ShapeMapDisplayData> perShapeGraph = std::vector<ShapeMapDisplayData>());

std::string MetaGraphReadWrite::getReadMessage(ReadStatus readStatus) {
    switch (readStatus) {
    case ReadStatus::OK:
        return "OK";
    case ReadStatus::WARN_BUGGY_VERSION:
        return "File version is buggy";
    case ReadStatus::WARN_CONVERTED:
        return "File was converted from an older version";
    case ReadStatus::NOT_A_GRAPH:
        return "Not a MetaGraph file";
    case ReadStatus::DAMAGED_FILE:
        return "Damaged file";
    case ReadStatus::DISK_ERROR:
        return "Disk error";
    case ReadStatus::NEWER_VERSION:
        return "MetaGraph file too new";
    case ReadStatus::DEPRECATED_VERSION:
        return "MetaGraph file too old";
    case ReadStatus::NOT_READ_YET:
        return "Reading interrupted";
    }
}
