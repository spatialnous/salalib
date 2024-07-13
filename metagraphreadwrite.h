// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/alllinemap.h"
#include "salalib/fileproperties.h"
#include "salalib/metagraph.h"
#include "salalib/pointmap.h"
#include "salalib/shapegraph.h"

#include "genlib/exceptions.h"
#include "salalib/shapemapgroupdata.h"

#include <istream>

namespace MetaGraphReadWrite {

    enum class ReadStatus {
        OK,
        WARN_BUGGY_VERSION,
        WARN_CONVERTED,
        NOT_A_GRAPH,
        DAMAGED_FILE,
        DISK_ERROR,
        NEWER_VERSION,
        DEPRECATED_VERSION,
        NOT_READ_YET
    };

    std::string getReadMessage(ReadStatus readStatus);

    class MetaGraphReadError : public depthmapX::BaseException {
      public:
        MetaGraphReadError(std::string message) : BaseException(message) {}
    };

    // editable, show, displayed attribute
    typedef std::tuple<bool, bool, int> ShapeMapDisplayData;

    struct MetaGraphData {
        ReadStatus readStatus = ReadStatus::NOT_READ_YET;
        int version;
        MetaGraph metaGraph;
        std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>> drawingFiles;
        std::vector<PointMap> pointMaps;
        std::vector<ShapeMap> dataMaps;
        std::vector<ShapeGraph> shapeGraphs;

        // The all-line map data are always placed at the end
        // but only valid if the index is found
        std::optional<AllLine::MapData> allLineMapData;

        // sala does not handle display data anymore, however they are still found in
        // the metagraphs, thus they are provided here for depthmapX (or other guis)
        struct DisplayData {
            int state = 0, viewClass = 0;
            bool showGrid = false, showText = false;
            std::vector<std::vector<ShapeMapDisplayData>> perDrawingMap;
            unsigned int displayedPointMap = -1;
            bool displayedPointMapProcessed = false; // P.K: never written but read. Old files?
            std::vector<int> perPointMap;
            unsigned int displayedDataMap = -1;
            std::vector<ShapeMapDisplayData> perDataMap;
            unsigned int displayedShapeGraph = -1;
            std::vector<ShapeMapDisplayData> perShapeGraph;
        } displayData;
    };

    void readHeader(std::istream &stream);
    QtRegion readRegion(std::istream &stream);

    std::tuple<std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>>,
               std::vector<std::vector<std::tuple<bool, bool, int>>>>
    readDrawingFiles(std::istream &stream);

    std::tuple<std::vector<ShapeGraph>, std::optional<AllLine::MapData>,
               std::vector<ShapeMapDisplayData>, unsigned int>
    readShapeGraphs(std::istream &stream);

    template <typename ShapeGraphOrRef>
    bool writeShapeGraphs(std::ofstream &stream, const std::vector<ShapeGraphOrRef> &shapeGraphs,
                          const std::optional<AllLine::MapData> allLineMapData,
                          const std::vector<std::tuple<bool, bool, int>> perShapeGraph,
                          const unsigned int displayedShapeGraph);

    std::tuple<std::vector<ShapeMap>, std::vector<std::tuple<bool, bool, int>>, unsigned int>
    readDataMaps(std::istream &stream);

    template <typename ShapeMapOrRef>
    bool writeDataMaps(
        std::ofstream &stream, const std::vector<ShapeMapOrRef> &dataMaps,
        const std::vector<ShapeMapDisplayData> displayData = std::vector<ShapeMapDisplayData>(),
        const unsigned int displayedMap = 0);

    std::tuple<std::vector<PointMap>, std::vector<int>, unsigned int>
    readPointMaps(std::istream &stream, QtRegion defaultRegion);

    template <typename PointMapOrRef>
    bool writePointMaps(std::ofstream &stream, const std::vector<PointMapOrRef> &pointMaps,
                        const std::vector<int> displayData = std::vector<int>(),
                        const unsigned int displayedMap = 0);

    template <typename ShapeMapOrRef>
    bool writeSpacePixels(std::ostream &stream, const std::vector<ShapeMapOrRef> &spacePixels,
                          const std::vector<std::tuple<bool, bool, int>> displayData);
    std::streampos skipVirtualMem(std::istream &stream);

    MetaGraphData readFromFile(const std::string &filename);
    MetaGraphData readFromStream(std::istream &stream);
    int write(const std::string &filename, const MetaGraphData &mgd);

    template <typename PointMapOrRef, typename ShapeMapOrRef, typename ShapeGraphOrRef>
    int write(
        const std::string &filename,
        // MetaGraph Data
        const int version, const std::string &name, const QtRegion &region,
        const FileProperties &fileProperties,
        const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMapOrRef>>> &drawingFiles,
        const std::vector<PointMapOrRef> &pointMaps, const std::vector<ShapeMapOrRef> &dataMaps,
        const std::vector<ShapeGraphOrRef> &shapeGraphs,
        const std::optional<AllLine::MapData> allLineMapData,
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
}; // namespace MetaGraphReadWrite

//#include "metagraphreadwrite_impl.h"
