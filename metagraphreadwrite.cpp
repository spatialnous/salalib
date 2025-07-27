// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "metagraphreadwrite.hpp"

#include "alllinemap.hpp"
#include "mgraph_consts.hpp"
#include "shapemapgroupdata.hpp"

#include "genlib/readwritehelpers.hpp"
#include "genlib/stringutils.hpp"

#include <fstream>

namespace {
    // old Depthmap display information, left here to allow reading
    // metagraph files
    enum {
        MG_State_NONE = 0x0000,
        MG_State_LATTICEMAPS = 0x0002,
        MG_State_LINEDATA = 0x0004,
        MG_State_ANGULARGRAPH = 0x0010,
        MG_State_DATAMAPS = 0x0020,
        MG_State_AXIALLINES = 0x0040,
        MG_State_SHAPEGRAPHS = 0x0100,
        MG_State_BUGGY = 0x8000
    };

    // These allow for the functions below to accept both maps (LatticeMap, Shapemap etc)
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

Region4f MetaGraphReadWrite::readRegion(std::istream &stream) {
    Region4f region;
    stream.read(reinterpret_cast<char *>(&region), sizeof(region));
    return region;
}

std::tuple<std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>>,
           std::vector<std::vector<std::tuple<bool, bool, int>>>>
MetaGraphReadWrite::readDrawingFiles(std::istream &stream) {
    int count;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));
    auto ucount = static_cast<size_t>(count);

    std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>> drawingFiles(ucount);
    std::vector<std::vector<std::tuple<bool, bool, int>>> displayData(ucount);
    for (size_t i = 0; i < ucount; i++) {
        drawingFiles[i].first.readInNameAndRegion(stream);

        std::tie(drawingFiles[i].second, displayData[i]) =
            ShapeMapGroupData::readSpacePixels(stream);
    }

    return std::make_tuple(std::move(drawingFiles), std::move(displayData));
}

MetaGraphReadWrite::MetaGraphData MetaGraphReadWrite::readFromStream(std::istream &stream) {

    MetaGraphData mgd;

    char header[3];
    stream.read(header, 3);
    if (stream.fail() || header[0] != 'g' || header[1] != 'r' || header[2] != 'f') {
        mgd.readWriteStatus = ReadWriteStatus::NOT_A_GRAPH;
        return mgd;
    }

    stream.read(reinterpret_cast<char *>(&mgd.version), sizeof(mgd.version));
    if (mgd.version > METAGRAPH_VERSION) {
        mgd.readWriteStatus = ReadWriteStatus::NEWER_VERSION;
        return mgd;
    }
    if (mgd.version < METAGRAPH_VERSION) {
        mgd.readWriteStatus = ReadWriteStatus::DEPRECATED_VERSION;
        return mgd;
    }

    // have to use temporary state here as redraw attempt may come too early:
    int tempState = 0;
    stream.read(reinterpret_cast<char *>(&tempState), sizeof(tempState));
    stream.read(reinterpret_cast<char *>(&mgd.displayData.viewClass),
                sizeof(mgd.displayData.viewClass));
    stream.read(reinterpret_cast<char *>(&mgd.displayData.showGrid),
                sizeof(mgd.displayData.showGrid));
    stream.read(reinterpret_cast<char *>(&mgd.displayData.showText),
                sizeof(mgd.displayData.showText));

    // type codes: x --- properties
    //             v --- virtual graph (from versions below 70)
    //             n --- ngraph format
    //             l --- layer data
    //             p --- point data
    //             d --- data summary layers

    char type;
    stream.read(&type, 1);
    if (type == 'd') {
        // contains deprecated datalayers. sala should be able to
        // convert them into shapemaps
        mgd.readWriteStatus = ReadWriteStatus::DEPRECATED_VERSION;
        return mgd;
    }
    if (type == 'x') {
        mgd.metaGraph.fileProperties.read(stream);
        if (stream.eof()) {
            // erk... this shouldn't happen
            mgd.readWriteStatus = ReadWriteStatus::DAMAGED_FILE;
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
            mgd.readWriteStatus = ReadWriteStatus::DAMAGED_FILE;
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
        tempState |= MG_State_LINEDATA;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
        if (!stream.eof() && !stream.good()) {
            // erk... this shouldn't happen
            mgd.readWriteStatus = ReadWriteStatus::DAMAGED_FILE;
            return mgd;
        }
    }
    if (type == 'p') {
        std::tie(mgd.latticeMaps, mgd.displayData.perLatticeMap,
                 mgd.displayData.displayedLatticeMap) =
            MetaGraphReadWrite::readLatticeMaps(stream, mgd.metaGraph.region);
        tempState |= MG_State_LATTICEMAPS;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
        if (type == 'g') {
            // record on state of actual point map:
            mgd.displayData.displayedLatticeMapProcessed = true;

            if (!stream.eof()) {
                stream.read(&type, 1);
            }
        }
    }
    if (type == 'a') {
        tempState |= MG_State_ANGULARGRAPH;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
    }
    if (type == 'x') {
        std::tie(mgd.shapeGraphs, mgd.allLineMapData, mgd.displayData.perShapeGraph,
                 mgd.displayData.displayedShapeGraph) = MetaGraphReadWrite::readShapeGraphs(stream);
        tempState |= MG_State_SHAPEGRAPHS;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
    }
    if (type == 's') {
        std::tie(mgd.dataMaps, mgd.displayData.perDataMap, mgd.displayData.displayedDataMap) =
            readDataMaps(stream);
        tempState |= MG_State_DATAMAPS;
        if (!stream.eof()) {
            stream.read(&type, 1);
        }
    }
    mgd.displayData.state = tempState;

    mgd.readWriteStatus = ReadWriteStatus::OK;
    return mgd;
}

MetaGraphReadWrite::ReadWriteStatus MetaGraphReadWrite::writeToFile(const std::string &filename,
                                                                    const MetaGraphData &mgd) {
    auto &dd = mgd.displayData;
    return MetaGraphReadWrite::writeToFile(
        filename,
        // MetaGraph Data
        mgd.version, mgd.metaGraph.name, mgd.metaGraph.region, mgd.metaGraph.fileProperties,
        mgd.drawingFiles, mgd.latticeMaps, mgd.dataMaps, mgd.shapeGraphs, mgd.allLineMapData,
        // display data
        dd.state, dd.viewClass, dd.showGrid, dd.showText, dd.perDrawingMap, dd.displayedLatticeMap,
        dd.perLatticeMap, dd.displayedDataMap, dd.perDataMap, dd.displayedShapeGraph,
        dd.perShapeGraph);
}

MetaGraphReadWrite::ReadWriteStatus MetaGraphReadWrite::writeToStream(std::ostream &stream,
                                                                      const MetaGraphData &mgd) {
    auto &dd = mgd.displayData;
    return MetaGraphReadWrite::writeToStream(
        stream,
        // MetaGraph Data
        mgd.version, mgd.metaGraph.name, mgd.metaGraph.region, mgd.metaGraph.fileProperties,
        mgd.drawingFiles, mgd.latticeMaps, mgd.dataMaps, mgd.shapeGraphs, mgd.allLineMapData,
        // display data
        dd.state, dd.viewClass, dd.showGrid, dd.showText, dd.perDrawingMap, dd.displayedLatticeMap,
        dd.perLatticeMap, dd.displayedDataMap, dd.perDataMap, dd.displayedShapeGraph,
        dd.perShapeGraph);
}

std::streampos MetaGraphReadWrite::skipVirtualMem(std::istream &stream) {
    // it's graph virtual memory: skip it
    int nodes = -1;
    stream.read(reinterpret_cast<char *>(&nodes), sizeof(nodes));

    nodes *= 2;

    for (int i = 0; i < nodes; i++) {
        int connections;
        stream.read(reinterpret_cast<char *>(&connections), sizeof(connections));
        stream.seekg(stream.tellg() +
                     std::streamoff(static_cast<size_t>(connections) * sizeof(connections)));
    }
    return (stream.tellg());
}

std::tuple<std::vector<LatticeMap>, std::vector<int>, std::optional<unsigned int>>
MetaGraphReadWrite::readLatticeMaps(std::istream &stream, Region4f defaultRegion) {
    int displayedMap = -1;
    stream.read(reinterpret_cast<char *>(&displayedMap), sizeof(displayedMap));
    std::vector<LatticeMap> latticeMaps;
    std::vector<int> displayAttributes;
    int count;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (int i = 0; i < count; i++) {
        latticeMaps.push_back(LatticeMap(defaultRegion));
        auto [completed, displayedAttribute] = latticeMaps.back().read(stream);
        displayAttributes.push_back(displayedAttribute);
    }
    return std::make_tuple(std::move(latticeMaps), std::move(displayAttributes),
                           displayedMap < 0
                               ? std::nullopt
                               : std::make_optional(static_cast<unsigned int>(displayedMap)));
}

template <typename LatticeMapOrRef>
bool MetaGraphReadWrite::writeLatticeMaps(std::ostream &stream,
                                          const std::vector<LatticeMapOrRef> &latticeMaps,
                                          const std::vector<int> &displayData,
                                          const std::optional<unsigned int> &displayedMap) {
    int displayedLatticeMap = displayedMap.has_value() ? static_cast<int>(*displayedMap) : -1;
    stream.write(reinterpret_cast<const char *>(&displayedLatticeMap), sizeof(displayedLatticeMap));
    auto count = latticeMaps.size();
    stream.write(reinterpret_cast<const char *>(&count), sizeof(static_cast<int>(count)));
    auto it = displayData.begin();
    for (auto &latticeMap : latticeMaps) {
        getMapRef(std::forward<const LatticeMapOrRef>(latticeMap),
                  is_reference_wrapper<std::decay_t<const LatticeMapOrRef>>{})
            .write(stream, *it);
        it++;
    }
    return true;
}

std::tuple<std::vector<ShapeMap>, std::vector<std::tuple<bool, bool, int>>,
           std::optional<unsigned int>>
MetaGraphReadWrite::readDataMaps(std::istream &stream) {
    std::vector<ShapeMap> dataMaps;
    std::vector<std::tuple<bool, bool, int>> displayData;
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    int displayedMap = -1;
    stream.read(reinterpret_cast<char *>(&displayedMap), sizeof(displayedMap));
    // read maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int count = 0;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));

    for (size_t j = 0; j < static_cast<size_t>(count); j++) {
        dataMaps.emplace_back();
        auto [completed, editable, show, displayedAttribute] = dataMaps.back().read(stream);
        displayData.emplace_back(editable, show, displayedAttribute);
    }
    return std::make_tuple(std::move(dataMaps), std::move(displayData),
                           displayedMap < 0
                               ? std::nullopt
                               : std::make_optional(static_cast<unsigned int>(displayedMap)));
}

template <typename ShapeMapOrRef>
bool MetaGraphReadWrite::writeDataMaps(std::ostream &stream,
                                       const std::vector<ShapeMapOrRef> &dataMaps,
                                       const std::vector<ShapeMapDisplayData> &displayData,
                                       const std::optional<unsigned int> &displayedMap) {
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    int displayedDataMap = displayedMap.has_value() ? static_cast<int>(*displayedMap) : -1;
    stream.write(reinterpret_cast<const char *>(&displayedDataMap), sizeof(displayedDataMap));
    // write maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    auto count = static_cast<unsigned int>(dataMaps.size());
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
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
    const std::vector<std::tuple<bool, bool, int>> &displayData) {

    int count = static_cast<int>(spacePixels.size());
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
    auto it = displayData.begin();
    for (auto &spacePixel : spacePixels) {

        getMapRef(std::forward<const ShapeMapOrRef>(spacePixel),
                  is_reference_wrapper<std::decay_t<const ShapeMapOrRef>>{})
            .write(stream, *it);
        it++;
    }
    return true;
}

std::tuple<std::vector<ShapeGraph>, std::optional<AllLine::MapData>,
           std::vector<MetaGraphReadWrite::ShapeMapDisplayData>, std::optional<unsigned int>>
MetaGraphReadWrite::readShapeGraphs(std::istream &stream) {
    std::vector<ShapeGraph> shapeGraphs;
    std::vector<std::tuple<bool, bool, int>> displayData;
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    int displayedMap = -1;
    stream.read(reinterpret_cast<char *>(&displayedMap), sizeof(displayedMap));
    // read maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    unsigned int count = 0;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));

    for (size_t j = 0; j < static_cast<size_t>(count); j++) {
        shapeGraphs.emplace_back();
        auto [completed, editable, show, displayedAttribute] = shapeGraphs.back().read(stream);
        displayData.emplace_back(editable, show, displayedAttribute);
    }

    // P.K: ideally this should be read together with the
    // all-line map, but the way the graph file is structured
    // this is not possible
    // TODO: Fix on next graph file update

    std::optional<AllLine::MapData> allLineMapData = std::nullopt;
    for (size_t i = 0; i < shapeGraphs.size(); i++) {
        if (shapeGraphs[i].getMapType() == ShapeMap::ALLLINEMAP) {

            allLineMapData.emplace();

            // this is an index to look up the all line map, used by the UI to determine
            // if can make fewest line map note: it is not saved for historical
            // reasons will get confused by more than one all line map
            allLineMapData->index = i;

            // these are additional essentially for all line axial maps
            // should probably be kept *with* the all line axial map...
            dXreadwrite::readIntoVector(stream, allLineMapData->polyConnections);
            dXreadwrite::readIntoVector(stream, allLineMapData->radialLines);

            // there is currently only one:
            break;
        }
    }
    if (!allLineMapData.has_value()) {
        // P.K. This is just a dummy read to cover cases where there is no All-Line
        // Map (empty vectors are still written) The below is taken from pmemvec<T>::read

        // READ / WRITE USES 32-bit LENGTHS (number of elements)
        // n.b., do not change this to size_t as it will cause 32-bit to 64-bit
        // conversion problems
        unsigned int length;
        stream.read(reinterpret_cast<char *>(&length), sizeof(unsigned int));
        stream.read(reinterpret_cast<char *>(&length), sizeof(unsigned int));
    }
    return std::make_tuple(
        std::move(shapeGraphs), std::move(allLineMapData), std::move(displayData),
        displayedMap < 0 ? std::nullopt
                         : std::make_optional(static_cast<unsigned int>(displayedMap)));
}

template <typename ShapeGraphOrRef>
bool MetaGraphReadWrite::writeShapeGraphs(
    std::ostream &stream, const std::vector<ShapeGraphOrRef> &shapeGraphs,
    const std::optional<AllLine::MapData> &allLineMapData,
    const std::vector<std::tuple<bool, bool, int>> &displayData,
    const std::optional<unsigned int> &displayedMap) {
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    int displayedShapeGraph = displayedMap.has_value() ? static_cast<int>(*displayedMap) : -1;
    stream.write(reinterpret_cast<const char *>(&displayedShapeGraph), sizeof(displayedShapeGraph));
    // write maps
    // n.b. -- do not change to size_t as will cause 32-bit to 64-bit conversion
    // problems
    auto count = static_cast<unsigned int>(shapeGraphs.size());
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
    auto it = displayData.begin();
    for (auto &shapeGraphPtr : shapeGraphs) {
        getMapRef(std::forward<const ShapeGraphOrRef>(shapeGraphPtr),
                  is_reference_wrapper<std::decay_t<const ShapeGraphOrRef>>{})
            .write(stream, *it);
        it++;
    }

    if (!allLineMapData.has_value()) {
        // There's still a reference to this data in the metagraph,
        // even if no all-line map is present
        unsigned int length = 0;
        stream.write(reinterpret_cast<const char *>(&length), sizeof(length));
        stream.write(reinterpret_cast<const char *>(&length), sizeof(length));
    } else {
        dXreadwrite::writeVector(stream, allLineMapData->polyConnections);
        dXreadwrite::writeVector(stream, allLineMapData->radialLines);
    }
    return true;
}

template <typename LatticeMapOrRef, typename ShapeMapOrRef, typename ShapeGraphOrRef>
MetaGraphReadWrite::ReadWriteStatus MetaGraphReadWrite::writeToFile(
    const std::string &filename,
    // MetaGraph Data
    const int version, const std::string &name, const Region4f &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMapOrRef>>> &drawingFiles,
    const std::vector<LatticeMapOrRef> &latticeMaps, const std::vector<ShapeMapOrRef> &dataMaps,
    const std::vector<ShapeGraphOrRef> &shapeGraphs,
    const std::optional<AllLine::MapData> &allLineMapData,
    // display data
    const int state, const int viewClass, const bool showGrid, const bool showText,
    const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap,
    const std::optional<unsigned int> &displayedLatticeMap, const std::vector<int> &perLatticeMap,
    const std::optional<unsigned int> &displayedDataMap,
    const std::vector<ShapeMapDisplayData> &perDataMap,
    const std::optional<unsigned int> &displayedShapeGraph,
    const std::vector<ShapeMapDisplayData> &perShapeGraph) {

    std::ofstream stream;

    // As of MetaGraph version 70 the disk caching has been removed
    stream.open(filename.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
    if (stream.fail()) {
        if (stream.rdbuf()->is_open()) {
            stream.close();
        }
        return ReadWriteStatus::DISK_ERROR;
    }
    auto result = writeToStream(stream,
                                // MetaGraph Data
                                version, name, region, fileProperties, drawingFiles, latticeMaps,
                                dataMaps, shapeGraphs, allLineMapData,
                                // display data
                                state, viewClass, showGrid, showText, perDrawingMap,
                                displayedLatticeMap, perLatticeMap, displayedDataMap, perDataMap,
                                displayedShapeGraph, perShapeGraph);

    stream.close();
    return result;
}

template <typename LatticeMapOrRef, typename ShapeMapOrRef, typename ShapeGraphOrRef>
MetaGraphReadWrite::ReadWriteStatus MetaGraphReadWrite::writeToStream(
    std::ostream &stream,
    // MetaGraph Data
    const int version, const std::string &name, const Region4f &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMapOrRef>>> &drawingFiles,
    const std::vector<LatticeMapOrRef> &latticeMaps, const std::vector<ShapeMapOrRef> &dataMaps,
    const std::vector<ShapeGraphOrRef> &shapeGraphs,
    const std::optional<AllLine::MapData> &allLineMapData,
    // display data
    const int state, const int viewClass, const bool showGrid, const bool showText,
    const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap,
    const std::optional<unsigned int> &displayedLatticeMap, const std::vector<int> &perLatticeMap,
    const std::optional<unsigned int> &displayedDataMap,
    const std::vector<ShapeMapDisplayData> &perDataMap,
    const std::optional<unsigned int> &displayedShapeGraph,
    const std::vector<ShapeMapDisplayData> &perShapeGraph) {

    char type;

    stream.write("grf", 3);
    stream.write(reinterpret_cast<const char *>(&version), sizeof(version));

    stream.write(reinterpret_cast<const char *>(&state), sizeof(state));
    stream.write(reinterpret_cast<const char *>(&viewClass), sizeof(viewClass));

    stream.write(reinterpret_cast<const char *>(&showGrid), sizeof(showGrid));
    stream.write(reinterpret_cast<const char *>(&showText), sizeof(showText));

    type = 'x';
    stream.write(&type, 1);
    fileProperties.write(stream);

    if (!drawingFiles.empty()) {
        type = 'l';
        stream.write(&type, 1);
        dXstring::writeString(stream, name);
        stream.write(reinterpret_cast<const char *>(&region), sizeof(region));

        int count = static_cast<int>(drawingFiles.size());
        stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
        if (perDrawingMap.empty()) {
            for (auto &spacePixel : drawingFiles) {
                spacePixel.first.writeOutNameAndRegion(stream);
                std::vector<ShapeMapDisplayData> displayData(spacePixel.second.size());
                for (auto &dd : displayData) {
                    std::get<0>(dd) = true;
                    std::get<1>(dd) = true;
                    std::get<2>(dd) = -1;
                }
                writeSpacePixels(stream, spacePixel.second, displayData);
            }
        } else {
            auto it = perDrawingMap.begin();
            for (auto &spacePixel : drawingFiles) {
                spacePixel.first.writeOutNameAndRegion(stream);
                writeSpacePixels(stream, spacePixel.second, *it);
                it++;
            }
        }
    }
    if (!latticeMaps.empty()) {
        type = 'p';
        stream.write(&type, 1);
        if (perLatticeMap.empty()) {
            std::vector<int> displayData(latticeMaps.size(), -1);
            writeLatticeMaps(stream, latticeMaps, displayData, displayedLatticeMap);
        } else {
            writeLatticeMaps(stream, latticeMaps, perLatticeMap, displayedLatticeMap);
        }
    }
    if (!shapeGraphs.empty()) {
        type = 'x';
        stream.write(&type, 1);
        if (perShapeGraph.empty()) {
            std::vector<ShapeMapDisplayData> displayData(shapeGraphs.size());
            for (auto &dd : displayData) {
                std::get<0>(dd) = true;
                std::get<1>(dd) = true;
                std::get<2>(dd) = -1;
            }
            writeShapeGraphs(stream, shapeGraphs, allLineMapData, displayData, displayedShapeGraph);
        } else {
            writeShapeGraphs(stream, shapeGraphs, allLineMapData, perShapeGraph,
                             displayedShapeGraph);
        }
    }
    if (!dataMaps.empty()) {
        type = 's';
        stream.write(&type, 1);
        if (perDataMap.empty()) {
            std::vector<ShapeMapDisplayData> displayData(dataMaps.size());
            for (auto &dd : displayData) {
                std::get<0>(dd) = true;
                std::get<1>(dd) = true;
                std::get<2>(dd) = -1;
            }
            writeDataMaps(stream, dataMaps, displayData, std::nullopt);
        } else {
            writeDataMaps(stream, dataMaps, perDataMap, displayedDataMap);
        }
    }

    return ReadWriteStatus::OK;
}

// these are just explicit instantiations of the write function to allow the linker
// to find them when required
template MetaGraphReadWrite::ReadWriteStatus
MetaGraphReadWrite::writeToFile<LatticeMap, ShapeMap, ShapeGraph>(
    const std::string &filename,
    // MetaGraph Data
    const int version, const std::string &name, const Region4f &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>> &drawingFiles,
    const std::vector<LatticeMap> &latticeMaps, const std::vector<ShapeMap> &dataMaps,
    const std::vector<ShapeGraph> &shapeGraphs,
    const std::optional<AllLine::MapData> &allLineMapData,
    // display data
    const int state, const int viewClass, const bool showGrid, const bool showText,
    const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap,
    const std::optional<unsigned int> &displayedLatticeMap, const std::vector<int> &perLatticeMap,
    const std::optional<unsigned int> &displayedDataMap,
    const std::vector<ShapeMapDisplayData> &perDataMap,
    const std::optional<unsigned int> &displayedShapeGraph,
    const std::vector<ShapeMapDisplayData> &perShapeGraph);

template MetaGraphReadWrite::ReadWriteStatus
MetaGraphReadWrite::writeToFile<std::reference_wrapper<LatticeMap>,
                                std::reference_wrapper<ShapeMap>,
                                std::reference_wrapper<ShapeGraph>>(
    const std::string &filename,
    // MetaGraph Data
    const int version, const std::string &name, const Region4f &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<std::reference_wrapper<ShapeMap>>>>
        &drawingFiles,
    const std::vector<std::reference_wrapper<LatticeMap>> &latticeMaps,
    const std::vector<std::reference_wrapper<ShapeMap>> &dataMaps,
    const std::vector<std::reference_wrapper<ShapeGraph>> &shapeGraphs,
    const std::optional<AllLine::MapData> &allLineMapData,
    // display data
    const int state, const int viewClass, const bool showGrid, const bool showText,
    const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap,
    const std::optional<unsigned int> &displayedLatticeMap, const std::vector<int> &perLatticeMap,
    const std::optional<unsigned int> &displayedDataMap,
    const std::vector<ShapeMapDisplayData> &perDataMap,
    const std::optional<unsigned int> &displayedShapeGraph,
    const std::vector<ShapeMapDisplayData> &perShapeGraph);

template MetaGraphReadWrite::ReadWriteStatus
MetaGraphReadWrite::writeToStream<LatticeMap, ShapeMap, ShapeGraph>(
    std::ostream &stream,
    // MetaGraph Data
    const int version, const std::string &name, const Region4f &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>> &drawingFiles,
    const std::vector<LatticeMap> &latticeMaps, const std::vector<ShapeMap> &dataMaps,
    const std::vector<ShapeGraph> &shapeGraphs,
    const std::optional<AllLine::MapData> &allLineMapData,
    // display data
    const int state, const int viewClass, const bool showGrid, const bool showText,
    const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap,
    const std::optional<unsigned int> &displayedLatticeMap, const std::vector<int> &perLatticeMap,
    const std::optional<unsigned int> &displayedDataMap,
    const std::vector<ShapeMapDisplayData> &perDataMap,
    const std::optional<unsigned int> &displayedShapeGraph,
    const std::vector<ShapeMapDisplayData> &perShapeGraph);

template MetaGraphReadWrite::ReadWriteStatus
MetaGraphReadWrite::writeToStream<std::reference_wrapper<LatticeMap>,
                                  std::reference_wrapper<ShapeMap>,
                                  std::reference_wrapper<ShapeGraph>>(
    std::ostream &stream,
    // MetaGraph Data
    const int version, const std::string &name, const Region4f &region,
    const FileProperties &fileProperties,
    const std::vector<std::pair<ShapeMapGroupData, std::vector<std::reference_wrapper<ShapeMap>>>>
        &drawingFiles,
    const std::vector<std::reference_wrapper<LatticeMap>> &latticeMaps,
    const std::vector<std::reference_wrapper<ShapeMap>> &dataMaps,
    const std::vector<std::reference_wrapper<ShapeGraph>> &shapeGraphs,
    const std::optional<AllLine::MapData> &allLineMapData,
    // display data
    const int state, const int viewClass, const bool showGrid, const bool showText,
    const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap,
    const std::optional<unsigned int> &displayedLatticeMap, const std::vector<int> &perLatticeMap,
    const std::optional<unsigned int> &displayedDataMap,
    const std::vector<ShapeMapDisplayData> &perDataMap,
    const std::optional<unsigned int> &displayedShapeGraph,
    const std::vector<ShapeMapDisplayData> &perShapeGraph);

std::string MetaGraphReadWrite::getReadMessage(ReadWriteStatus readStatus) {
    switch (readStatus) {
    case ReadWriteStatus::OK:
        return "OK";
    case ReadWriteStatus::WARN_BUGGY_VERSION:
        return "File version is buggy";
    case ReadWriteStatus::WARN_CONVERTED:
        return "File was converted from an older version";
    case ReadWriteStatus::NOT_A_GRAPH:
        return "Not a MetaGraph file";
    case ReadWriteStatus::DAMAGED_FILE:
        return "Damaged file";
    case ReadWriteStatus::DISK_ERROR:
        return "Disk error";
    case ReadWriteStatus::NEWER_VERSION:
        return "MetaGraph file too new";
    case ReadWriteStatus::DEPRECATED_VERSION:
        return "MetaGraph file too old";
    case ReadWriteStatus::NOT_READ_YET:
        return "Reading interrupted";
    }
    return "<Unknown state>";
}
