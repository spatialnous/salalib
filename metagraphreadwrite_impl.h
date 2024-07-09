// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/p2dpoly.h"
#include "salalib/fileproperties.h"
#include "salalib/shapemapgroupdata.h"

#include <fstream>
#include <string>

namespace MetaGraphReadWrite {

    typedef std::tuple<bool, bool, int> ShapeMapDisplayData;

    //    template <typename PointMapOrRef, typename ShapeMapOrRef, typename ShapeGraphOrRef>
    //    int
    //    write(const std::string &filename,
    //          // MetaGraph Data
    //          const int version, const std::string &name, const QtRegion &region,
    //          const FileProperties &fileProperties,
    //          const std::vector<std::pair<ShapeMapGroupData, std::vector<ShapeMapOrRef>>>
    //          &drawingFiles, const std::vector<PointMapOrRef> &pointMaps, const
    //          std::vector<ShapeMapOrRef> &dataMaps, const std::vector<ShapeGraphOrRef>
    //          &shapeGraphs, const std::optional<size_t> allLineMapIdx,
    //          // display data
    //          const int state, const int viewClass, const bool showGrid, const bool showText,
    //          const std::vector<std::vector<ShapeMapDisplayData>> perDrawingMap,
    //          const unsigned int displayedPointMap, const std::vector<int> perPointMap,
    //          const unsigned int displayedDataMap, const std::vector<ShapeMapDisplayData>
    //          perDataMap, const unsigned int displayedShapeGraph, const
    //          std::vector<ShapeMapDisplayData> perShapeGraph) {

    //        enum {
    //            OK,
    //            WARN_BUGGY_VERSION,
    //            WARN_CONVERTED,
    //            NOT_A_GRAPH,
    //            DAMAGED_FILE,
    //            DISK_ERROR,
    //            NEWER_VERSION,
    //            DEPRECATED_VERSION
    //        };
    //        enum {
    //            NONE = 0x0000,
    //            POINTMAPS = 0x0002,
    //            LINEDATA = 0x0004,
    //            ANGULARGRAPH = 0x0010,
    //            DATAMAPS = 0x0020,
    //            AXIALLINES = 0x0040,
    //            SHAPEGRAPHS = 0x0100,
    //            BUGGY = 0x8000
    //        };

    //        std::ofstream stream;

    //        char type;

    //        // As of MetaGraph version 70 the disk caching has been removed
    //        stream.open(filename.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);
    //        if (stream.fail()) {
    //            if (stream.rdbuf()->is_open()) {
    //                stream.close();
    //            }
    //            return DISK_ERROR;
    //        }
    //        stream.write("grf", 3);
    //        stream.write((char *)&version, sizeof(version));

    //        stream.write((char *)&state, sizeof(state));
    //        stream.write((char *)&viewClass, sizeof(viewClass));

    //        stream.write((char *)&showGrid, sizeof(showGrid));
    //        stream.write((char *)&showText, sizeof(showText));

    //        type = 'x';
    //        stream.write(&type, 1);
    //        fileProperties.write(stream);

    //        if (state & LINEDATA) {
    //            type = 'l';
    //            stream.write(&type, 1);
    //            dXstring::writeString(stream, name);
    //            stream.write((char *)&region, sizeof(region));

    //            int count = static_cast<int>(drawingFiles.size());
    //            stream.write((char *)&count, sizeof(count));
    //            auto it = perDrawingMap.begin();
    //            for (auto &spacePixel : drawingFiles) {
    //                spacePixel.first.writeOutName(stream);
    //                writeSpacePixels(stream, spacePixel.second, *it);
    //                it++;
    //            }
    //        }
    //        if (state & POINTMAPS) {
    //            type = 'p';
    //            stream.write(&type, 1);
    //            writePointMaps(stream, pointMaps, perPointMap, displayedPointMap);
    //        }
    //        if (state & SHAPEGRAPHS) {
    //            type = 'x';
    //            stream.write(&type, 1);
    //            writeShapeGraphs(stream, shapeGraphs, allLineMapIdx, perShapeGraph,
    //                             displayedShapeGraph);
    //        }
    //        if (state & DATAMAPS) {
    //            type = 's';
    //            stream.write(&type, 1);
    //            writeDataMaps(stream, dataMaps, perDataMap, displayedDataMap);
    //        }

    //        stream.close();
    //        return OK;
    //    }
} // namespace MetaGraphReadWrite
