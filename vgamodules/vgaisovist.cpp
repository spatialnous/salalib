// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaisovist.h"

#include "salalib/isovist.h"

AnalysisResult VGAIsovist::run(Communicator *comm, PointMap &map, bool simple_version) {
    map.m_hasIsovistAnalysis = false;

    AnalysisResult result;

    // note, BSP tree plays with comm counting...
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }
    BSPNode bspRoot = makeBSPtree(comm, m_boundaryShapes);

    AttributeTable &attributes = map.getAttributeTable();

    if (comm)
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }
    int count = 0;
    auto cols = createAttributes(attributes, simple_version);
    for (const auto &col : cols) {
        result.addAttribute(col.first);
    }

    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (map.getPoint(curs).filled()) {
                count++;
                if (map.getPoint(curs).contextfilled() && !curs.iseven()) {
                    continue;
                }
                Isovist isovist;
                isovist.makeit(&bspRoot, map.depixelate(curs), map.getRegion(), 0, 0);

                AttributeRow &row = attributes.getRow(AttributeKey(curs));
                setData(isovist, row, cols, simple_version);
                Node &node = map.getPoint(curs).getNode();
                std::vector<PixelRef> *occ = node.m_occlusion_bins;
                for (size_t k = 0; k < 32; k++) {
                    occ[k].clear();
                    node.bin(static_cast<int>(k)).setOccDistance(0.0f);
                }
                for (size_t k = 0; k < isovist.getOcclusionPoints().size(); k++) {
                    const PointDist &pointdist = isovist.getOcclusionPoints().at(k);
                    int bin = whichbin(pointdist.m_point - map.depixelate(curs));
                    // only occlusion bins with a certain distance recorded (arbitrary scale note!)
                    if (pointdist.m_dist > 1.5) {
                        PixelRef pix = map.pixelate(pointdist.m_point);
                        if (pix != curs) {
                            occ[bin].push_back(pix);
                        }
                    }
                    node.bin(bin).setOccDistance(static_cast<float>(pointdist.m_dist));
                }
            }
            if (comm) {
                if (qtimer(atime, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                }
            }
        }
    }
    map.m_hasIsovistAnalysis = true;

    result.completed = true;

    return result;
}

std::vector<std::pair<std::string, int>> VGAIsovist::createAttributes(AttributeTable &table,
                                                                      bool simple_version) {
    std::vector<std::pair<std::string, int>> cols;

    int col = table.getOrInsertColumn(Column::ISOVIST_AREA);
    cols.emplace_back(Column::ISOVIST_AREA, col);

    if (!simple_version) {
        col = table.getOrInsertColumn(Column::ISOVIST_COMPACTNESS);
        cols.emplace_back(Column::ISOVIST_COMPACTNESS, col);

        col = table.getOrInsertColumn(Column::ISOVIST_DRIFT_ANGLE);
        cols.emplace_back(Column::ISOVIST_DRIFT_ANGLE, col);

        col = table.getOrInsertColumn(Column::ISOVIST_DRIFT_MAGNITUDE);
        cols.emplace_back(Column::ISOVIST_DRIFT_MAGNITUDE, col);

        col = table.getOrInsertColumn(Column::ISOVIST_MIN_RADIAL);
        cols.emplace_back(Column::ISOVIST_MIN_RADIAL, col);

        col = table.getOrInsertColumn(Column::ISOVIST_MAX_RADIAL);
        cols.emplace_back(Column::ISOVIST_MAX_RADIAL, col);

        col = table.getOrInsertColumn(Column::ISOVIST_OCCLUSIVITY);
        cols.emplace_back(Column::ISOVIST_OCCLUSIVITY, col);

        col = table.getOrInsertColumn(Column::ISOVIST_PERIMETER);
        cols.emplace_back(Column::ISOVIST_PERIMETER, col);
    }
    return cols;
}

std::set<std::string> VGAIsovist::setData(Isovist &isovist, AttributeRow &row,
                                          std::vector<std::pair<std::string, int>> cols,
                                          bool simple_version) {
    std::set<std::string> newColumns;
    auto [centroid, area] = isovist.getCentroidArea();
    auto [driftmag, driftang] = isovist.getDriftData();
    double perimeter = isovist.getPerimeter();

    auto colIt = cols.begin();
    row.setValue(colIt->second, float(area));

    if (!simple_version) {
        ++colIt;
        row.setValue(colIt->second, float(4.0 * M_PI * area / (perimeter * perimeter)));

        ++colIt;
        row.setValue(colIt->second, float(180.0 * driftang / M_PI));

        ++colIt;
        row.setValue(colIt->second, float(driftmag));

        ++colIt;
        row.setValue(colIt->second, float(isovist.getMinRadial()));

        ++colIt;
        row.setValue(colIt->second, float(isovist.getMaxRadial()));

        ++colIt;
        row.setValue(colIt->second, float(isovist.getOccludedPerimeter()));

        ++colIt;
        row.setValue(colIt->second, float(perimeter));
    }
    return newColumns;
}

BSPNode VGAIsovist::makeBSPtree(Communicator *communicator,
                                const std::vector<SalaShape> &boundaryShapes) {
    std::vector<Line> partitionlines;
    for (const auto &shape : boundaryShapes) {
        std::vector<Line> newLines = shape.getAsLines();
        for (const Line &line : newLines) {
            if (line.length() > 0.0) {
                partitionlines.push_back(line);
            }
        }
    }

    BSPNode bspRoot;
    if (partitionlines.size()) {

        time_t atime = 0;
        if (communicator) {
            communicator->CommPostMessage(Communicator::NUM_RECORDS,
                                          static_cast<int>(partitionlines.size()));
            qtimer(atime, 0);
        }

        BSPTree::make(communicator, atime, partitionlines, &bspRoot);
    }

    return bspRoot;
}
