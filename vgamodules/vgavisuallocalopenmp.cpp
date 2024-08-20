// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisuallocalopenmp.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

AnalysisResult VGAVisualLocalOpenMP::run(Communicator *comm) {

#if !defined(_OPENMP)
    std::cerr << "OpenMP NOT available, only running on a single core" << std::endl;
    m_forceCommUpdatesMasterThread = false;
#else
    if (m_limitToThreads.has_value()) {
        omp_set_num_threads(m_limitToThreads.value());
    }
#endif

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    AttributeTable &attributes = m_map.getAttributeTable();

    std::vector<PixelRef> filled;
    std::vector<AttributeRow *> rows;

    for (size_t i = 0; i < m_map.getCols(); i++) {
        for (size_t j = 0; j < m_map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (m_map.getPoint(curs).filled()) {
                filled.push_back(curs);
                rows.push_back(attributes.getRowPtr(AttributeKey(curs)));
            }
        }
    }

    int count = 0;

    count = 0;

    std::vector<DataPoint> colData(filled.size());

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_STEPS, 1);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
        comm->CommPostMessage(Communicator::NUM_RECORDS, filled.size());
    }
    std::vector<std::set<int>> hoods(filled.size());

    int i, n = int(filled.size());
    std::map<PixelRef, int> refToFilled;
    for (i = 0; i < n; ++i) {
        refToFilled.insert(std::make_pair(filled[size_t(i)], i));
    }

#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif
    for (i = 0; i < n; ++i) {
        Point &p = m_map.getPoint(filled[size_t(i)]);
        std::set<PixelRef> neighbourhood = getNeighbourhood(p.getNode());
        for (auto &neighbour : neighbourhood) {
            if (m_map.getPoint(neighbour).hasNode()) {
                hoods[size_t(i)].insert(refToFilled[neighbour]);
            }
        }
    }

#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif
    for (i = 0; i < n; ++i) {

        DataPoint &dp = colData[i];

        Point &p = m_map.getPoint(filled[size_t(i)]);
        if ((p.contextfilled() && !filled[size_t(i)].iseven())) {
            count++;
            continue;
        }

        // This is much easier to do with a straight forward list:
        std::set<int> &neighbourhood = hoods[size_t(i)];
        std::set<int> totalneighbourhood;
        int cluster = 0;
        float control = 0.0f;

        for (auto &neighbour : neighbourhood) {
            std::set<int> &retneighbourhood = hoods[size_t(neighbour)];
            std::set<int> intersect;
            std::set_intersection(neighbourhood.begin(), neighbourhood.end(),
                                  retneighbourhood.begin(), retneighbourhood.end(),
                                  std::inserter(intersect, intersect.begin()));
            totalneighbourhood.insert(retneighbourhood.begin(), retneighbourhood.end());
            control += 1.0f / float(retneighbourhood.size());
            cluster += intersect.size();
        }
#if defined(_OPENMP)
#pragma omp critical(add_to_col)
#endif
        {
            if (neighbourhood.size() > 1) {
                dp.cluster =
                    float(cluster / double(neighbourhood.size() * (neighbourhood.size() - 1.0)));
                dp.control = float(control);
                dp.controllability =
                    float(double(neighbourhood.size()) / double(totalneighbourhood.size()));
            } else {
                dp.cluster = -1.0f;
                dp.control = -1.0f;
                dp.controllability = -1.0f;
            }
        }

#if defined(_OPENMP)
#pragma omp atomic
#endif
        count++; // <- increment count

#if defined(_OPENMP)
        // only executed by the main thread if requested
        if (!m_forceCommUpdatesMasterThread || omp_get_thread_num() == 0)
#endif
            if (comm) {
                if (qtimer(atime, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                }
            }
    }

    AnalysisResult result;

    std::string clusterColName = Column::VISUAL_CLUSTERING_COEFFICIENT;
    std::string controlColName = Column::VISUAL_CONTROL;
    std::string controllabilityColName = Column::VISUAL_CONTROLLABILITY;
    int clusterCol = attributes.insertOrResetColumn(clusterColName);
    result.addAttribute(clusterColName);
    int controlCol = attributes.insertOrResetColumn(controlColName);
    result.addAttribute(controlColName);
    int controllabilityCol = attributes.insertOrResetColumn(controllabilityColName);
    result.addAttribute(controllabilityColName);

    auto dataIter = colData.begin();
    for (auto row : rows) {
        row->setValue(clusterCol, dataIter->cluster);
        row->setValue(controlCol, dataIter->control);
        row->setValue(controllabilityCol, dataIter->controllability);
        dataIter++;
    }

    result.completed = true;

    return result;
}

std::set<PixelRef> VGAVisualLocalOpenMP::getNeighbourhood(const Node &node) const {
    std::set<PixelRef> hood;
    for (int i = 0; i < 32; i++) {
        const Bin &bin = node.bin(i);
        for (auto pixVec : bin.pixelVecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.dir) <= pixVec.end().col(bin.dir);) {
                hood.insert(pix);
                pix.move(bin.dir);
            }
        }
    }
    return hood;
}
