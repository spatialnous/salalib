// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaisovist.h"

#include "../isovist.h"

AnalysisResult VGAIsovist::run(Communicator *comm) {

    // note, BSP tree plays with comm counting...
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }
    BSPNode bspRoot = makeBSPtree(comm, m_boundaryShapes);

    if (comm)
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }
    size_t count = 0;

    AnalysisResult result(createAttributes(m_simpleVersion), m_map.getFilledPointCount());

    for (size_t i = 0; i < m_map.getCols(); i++) {
        for (size_t j = 0; j < m_map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (m_map.getPoint(curs).filled()) {
                if (m_map.getPoint(curs).contextfilled() && !curs.iseven()) {
                    count++;
                    continue;
                }
                Isovist isovist;
                isovist.makeit(&bspRoot, m_map.depixelate(curs), m_map.getRegion(), 0, 0);

                setData(isovist, count, result, m_simpleVersion);
                Node &node = m_map.getPoint(curs).getNode();
                std::vector<PixelRef> *occ = node.occlusionBins;
                for (size_t k = 0; k < 32; k++) {
                    occ[k].clear();
                    node.bin(static_cast<int>(k)).setOccDistance(0.0f);
                }
                for (size_t k = 0; k < isovist.getOcclusionPoints().size(); k++) {
                    const PointDist &pointdist = isovist.getOcclusionPoints().at(k);
                    int bin = whichbin(pointdist.point - m_map.depixelate(curs));
                    // only occlusion bins with a certain distance recorded (arbitrary scale note!)
                    if (pointdist.dist > 1.5) {
                        PixelRef pix = m_map.pixelate(pointdist.point);
                        if (pix != curs) {
                            occ[bin].push_back(pix);
                        }
                    }
                    node.bin(bin).setOccDistance(static_cast<float>(pointdist.dist));
                }
                count++;
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
    }

    result.completed = true;

    return result;
}

std::vector<std::string> VGAIsovist::createAttributes(bool simpleVersion) const {
    std::vector<std::string> cols;

    cols.push_back(Column::ISOVIST_AREA);

    if (!simpleVersion) {
        cols.push_back(Column::ISOVIST_COMPACTNESS);
        cols.push_back(Column::ISOVIST_DRIFT_ANGLE);
        cols.push_back(Column::ISOVIST_DRIFT_MAGNITUDE);
        cols.push_back(Column::ISOVIST_MIN_RADIAL);
        cols.push_back(Column::ISOVIST_MAX_RADIAL);
        cols.push_back(Column::ISOVIST_OCCLUSIVITY);
        cols.push_back(Column::ISOVIST_PERIMETER);
    }
    return cols;
}

std::set<std::string> VGAIsovist::setData(Isovist &isovist, size_t &index, AnalysisResult &result,
                                          bool simpleVersion) const {
    std::set<std::string> newColumns;
    auto [centroid, area] = isovist.getCentroidArea();
    auto [driftmag, driftang] = isovist.getDriftData();
    double perimeter = isovist.getPerimeter();

    size_t currCol = 0;
    result.setValue(index, currCol, area);

    if (!simpleVersion) {
        ++currCol;
        result.setValue(index, currCol, 4.0 * M_PI * area / (perimeter * perimeter));

        ++currCol;
        result.setValue(index, currCol, 180.0 * driftang / M_PI);

        ++currCol;
        result.setValue(index, currCol, driftmag);

        ++currCol;
        result.setValue(index, currCol, isovist.getMinRadial());

        ++currCol;
        result.setValue(index, currCol, isovist.getMaxRadial());

        ++currCol;
        result.setValue(index, currCol, isovist.getOccludedPerimeter());

        ++currCol;
        result.setValue(index, currCol, perimeter);
    }
    return newColumns;
}

BSPNode VGAIsovist::makeBSPtree(Communicator *communicator,
                                const std::vector<SalaShape> &boundaryShapes) const {
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
