// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "alllinemap.hpp"
#include "fileproperties.hpp"
#include "metagraph.hpp"
#include "pointmap.hpp"
#include "shapegraph.hpp"

#include "genlib/exceptions.hpp"
#include "shapemapgroupdata.hpp"

#include <istream>

namespace MetaGraphReadWrite {

    enum class ReadWriteStatus {
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

    std::string getReadMessage(ReadWriteStatus readStatus);

    class MetaGraphReadError : public depthmapX::BaseException {
      public:
        MetaGraphReadError(std::string message) : BaseException(std::move(message)) {}
    };

    // editable, show, displayed attribute
    typedef std::tuple<bool, bool, int> ShapeMapDisplayData;

    struct MetaGraphData {
        ReadWriteStatus readWriteStatus = ReadWriteStatus::NOT_READ_YET;
        int version = 0;
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
            bool displayedPointMapProcessed = false; // P.K: never written but read. Old files?

          private:
            [[maybe_unused]] unsigned _padding0 : 1 * 8;
            [[maybe_unused]] unsigned _padding1 : 4 * 8;

          public:
            std::vector<std::vector<ShapeMapDisplayData>> perDrawingMap;
            std::optional<unsigned int> displayedPointMap = std::nullopt;
            std::vector<int> perPointMap;
            std::optional<unsigned int> displayedDataMap = std::nullopt;
            std::vector<ShapeMapDisplayData> perDataMap;
            std::optional<unsigned int> displayedShapeGraph = std::nullopt;
            std::vector<ShapeMapDisplayData> perShapeGraph;
            DisplayData()
                : _padding0(0), _padding1(0), perDrawingMap(), displayedPointMap(std::nullopt),
                  perPointMap(), displayedDataMap(std::nullopt), perDataMap(),
                  displayedShapeGraph(std::nullopt), perShapeGraph() {}
        } displayData;

        MetaGraphData()
            : readWriteStatus(ReadWriteStatus::NOT_READ_YET), version(0), metaGraph(),
              drawingFiles(), pointMaps(), dataMaps(), shapeGraphs(), allLineMapData(),
              displayData() {}
    };

    Region4f readRegion(std::istream &stream);

    std::tuple<std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMap>>>,
               std::vector<std::vector<std::tuple<bool, bool, int>>>>
    readDrawingFiles(std::istream &stream);

    std::tuple<std::vector<ShapeGraph>, std::optional<AllLine::MapData>,
               std::vector<ShapeMapDisplayData>, std::optional<unsigned int>>
    readShapeGraphs(std::istream &stream);

    template <typename ShapeGraphOrRef>
    bool writeShapeGraphs(std::ostream &stream, const std::vector<ShapeGraphOrRef> &shapeGraphs,
                          const std::optional<AllLine::MapData> &allLineMapData,
                          const std::vector<std::tuple<bool, bool, int>> &perShapeGraph,
                          const std::optional<unsigned int> &displayedMap);

    std::tuple<std::vector<ShapeMap>, std::vector<std::tuple<bool, bool, int>>,
               std::optional<unsigned int>>
    readDataMaps(std::istream &stream);

    template <typename ShapeMapOrRef>
    bool writeDataMaps(
        std::ostream &stream, const std::vector<ShapeMapOrRef> &dataMaps,
        const std::vector<ShapeMapDisplayData> &displayData = std::vector<ShapeMapDisplayData>(),
        const std::optional<unsigned int> &displayedMap = 0);

    std::tuple<std::vector<PointMap>, std::vector<int>, std::optional<unsigned int>>
    readPointMaps(std::istream &stream, Region4f defaultRegion);

    template <typename PointMapOrRef>
    bool writePointMaps(std::ostream &stream, const std::vector<PointMapOrRef> &pointMaps,
                        const std::vector<int> &displayData = std::vector<int>(),
                        const std::optional<unsigned int> &displayedMap = 0);

    template <typename ShapeMapOrRef>
    bool writeSpacePixels(std::ostream &stream, const std::vector<ShapeMapOrRef> &spacePixels,
                          const std::vector<std::tuple<bool, bool, int>> &displayData);
    std::streampos skipVirtualMem(std::istream &stream);

    MetaGraphData readFromFile(const std::string &filename);
    MetaGraphData readFromStream(std::istream &stream);

    ReadWriteStatus writeToFile(const std::string &filename, const MetaGraphData &mgd);

    template <typename PointMapOrRef, typename ShapeMapOrRef, typename ShapeGraphOrRef>
    ReadWriteStatus writeToFile(
        const std::string &filename,
        // MetaGraph Data
        const int version, const std::string &name, const Region4f &region,
        const FileProperties &fileProperties,
        const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMapOrRef>>> &drawingFiles,
        const std::vector<PointMapOrRef> &pointMaps, const std::vector<ShapeMapOrRef> &dataMaps,
        const std::vector<ShapeGraphOrRef> &shapeGraphs,
        const std::optional<AllLine::MapData> &allLineMapData,
        // display data
        const int state = 0, const int viewClass = 0, const bool showGrid = true,
        const bool showText = true,
        const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap =
            std::vector<std::vector<ShapeMapDisplayData>>(),
        const std::optional<unsigned int> &displayedPointMap = std::nullopt,
        const std::vector<int> &perPointMap = std::vector<int>(),
        const std::optional<unsigned int> &displayedDataMap = std::nullopt,
        const std::vector<ShapeMapDisplayData> &perDataMap = std::vector<ShapeMapDisplayData>(),
        const std::optional<unsigned int> &displayedShapeGraph = std::nullopt,
        const std::vector<ShapeMapDisplayData> &perShapeGraph = std::vector<ShapeMapDisplayData>());

    ReadWriteStatus writeToStream(std::ostream &stream, const MetaGraphData &mgd);

    template <typename PointMapOrRef, typename ShapeMapOrRef, typename ShapeGraphOrRef>
    ReadWriteStatus writeToStream(
        std::ostream &stream,
        // MetaGraph Data
        const int version, const std::string &name, const Region4f &region,
        const FileProperties &fileProperties,
        const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMapOrRef>>> &drawingFiles,
        const std::vector<PointMapOrRef> &pointMaps, const std::vector<ShapeMapOrRef> &dataMaps,
        const std::vector<ShapeGraphOrRef> &shapeGraphs,
        const std::optional<AllLine::MapData> &allLineMapData,
        // display data
        const int state = 0, const int viewClass = 0, const bool showGrid = true,
        const bool showText = true,
        const std::vector<std::vector<ShapeMapDisplayData>> &perDrawingMap =
            std::vector<std::vector<ShapeMapDisplayData>>(),
        const std::optional<unsigned int> &displayedPointMap = std::nullopt,
        const std::vector<int> &perPointMap = std::vector<int>(),
        const std::optional<unsigned int> &displayedDataMap = std::nullopt,
        const std::vector<ShapeMapDisplayData> &perDataMap = std::vector<ShapeMapDisplayData>(),
        const std::optional<unsigned int> &displayedShapeGraph = std::nullopt,
        const std::vector<ShapeMapDisplayData> &perShapeGraph = std::vector<ShapeMapDisplayData>());
} // namespace MetaGraphReadWrite

// #include "metagraphreadwrite_impl.hpp"
