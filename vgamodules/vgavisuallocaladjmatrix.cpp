// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisuallocaladjmatrix.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

AnalysisResult VGAVisualLocalAdjMatrix::run(Communicator *comm) {

#if !defined(_OPENMP)
    std::cerr << "OpenMP NOT available, only running on a single core" << std::endl;
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

    std::vector<DataPoint> col_data(filled.size());

    int i;
    const long N = long(filled.size());

    std::map<PixelRef, int> refToFilled;
    for (i = 0; i < N; ++i) {
        refToFilled.insert(std::make_pair(filled[size_t(i)], i));
    }

    std::vector<bool> hoods(N * N);

#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif
    for (i = 0; i < N; ++i) {
        Point &p = m_map.getPoint(filled[size_t(i)]);
        std::set<PixelRef> neighbourhood;
#if defined(_OPENMP)
#pragma omp critical(dumpNeighbourhood)
#endif
        { dumpNeighbourhood(p.getNode(), neighbourhood); }
        for (auto &neighbour : neighbourhood) {
            if (m_map.getPoint(neighbour).hasNode()) {
                hoods[long(i * N + refToFilled[neighbour])] = true;
            }
        }
    }

#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif
    for (i = 0; i < N; ++i) {

        DataPoint &dp = col_data[i];

        Point &p = m_map.getPoint(filled[size_t(i)]);
        if ((p.contextfilled() && !filled[size_t(i)].iseven()) || (m_gates_only)) {
            count++;
            continue;
        }

        std::vector<bool> totalHood(N);

        int cluster = 0;
        float control = 0.0f;

        int hoodSize = 0;
        for (int j = 0; j < N; j++) {
            if (hoods[i * N + j]) {
                hoodSize++;
                int retHood = 0;
                for (int k = 0; k < N; k++) {
                    if (hoods[j * N + k]) {
                        totalHood[k] = true;
                        retHood++;
                        if (hoods[i * N + k])
                            cluster++;
                    }
                }
                control += 1.0f / float(retHood);
            }
        }
        int totalReach = 0;
        for (int j = 0; j < N; j++) {
            if (totalHood[j])
                totalReach++;
        }
#if defined(_OPENMP)
#pragma omp critical(add_to_col)
#endif
        {
            if (hoodSize > 1) {
                dp.cluster = float(cluster / double(hoodSize * (hoodSize - 1.0)));
                dp.control = float(control);
                dp.controllability = float(double(hoodSize) / double(totalReach));
            } else {
                dp.cluster = -1.0f;
                dp.control = -1.0f;
                dp.controllability = -1;
            }
        }

#if defined(_OPENMP)
#pragma omp critical(count)
#endif
        {
            count++; // <- increment count
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

    AnalysisResult result;

    std::string cluster_col_name = Column::VISUAL_CLUSTERING_COEFFICIENT;
    std::string control_col_name = Column::VISUAL_CONTROL;
    std::string controllability_col_name = Column::VISUAL_CONTROLLABILITY;
    int cluster_col = attributes.insertOrResetColumn(cluster_col_name);
    result.addAttribute(cluster_col_name);
    int control_col = attributes.insertOrResetColumn(control_col_name);
    result.addAttribute(control_col_name);
    int controllability_col = attributes.insertOrResetColumn(controllability_col_name);
    result.addAttribute(controllability_col_name);

    auto dataIter = col_data.begin();
    for (auto row : rows) {
        row->setValue(cluster_col, dataIter->cluster);
        row->setValue(control_col, dataIter->control);
        row->setValue(controllability_col, dataIter->controllability);
        dataIter++;
    }

    result.completed = true;

    return result;
}

void VGAVisualLocalAdjMatrix::dumpNeighbourhood(Node &node, std::set<PixelRef> &hood) const {
    for (int i = 0; i < 32; i++) {
        Bin &bin = node.bin(i);
        for (auto pixVec : bin.m_pixel_vecs) {
            for (PixelRef pix = pixVec.start();
                 pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                hood.insert(pix);
                pix.move(bin.m_dir);
            }
        }
    }
}
