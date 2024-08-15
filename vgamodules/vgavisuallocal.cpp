// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisuallocal.h"

AnalysisResult VGAVisualLocal::run(Communicator *comm) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    AnalysisResult result({Column::VISUAL_CLUSTERING_COEFFICIENT, Column::VISUAL_CONTROL,
                           Column::VISUAL_CONTROLLABILITY},
                          m_map.getAttributeTable().getNumRows());

    auto cluster_col = result.getColumnIndex(Column::VISUAL_CLUSTERING_COEFFICIENT);
    auto control_col = result.getColumnIndex(Column::VISUAL_CONTROL);
    auto controllability_col = result.getColumnIndex(Column::VISUAL_CONTROLLABILITY);

    const auto refs = getRefVector(m_map.getAttributeTable());

    size_t count = 0;

    for (size_t i = 0; i < m_map.getCols(); i++) {
        for (size_t j = 0; j < m_map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (m_map.getPoint(curs).filled()) {
                if ((m_map.getPoint(curs).contextfilled() && !curs.iseven()) || (m_gates_only)) {
                    count++;
                    continue;
                }
                auto refIdx = getRefIdx(refs, curs);

                // This is much easier to do with a straight forward list:
                PixelRefVector neighbourhood;
                PixelRefVector totalneighbourhood;
                m_map.getPoint(curs).getNode().contents(neighbourhood);

                // only required to match previous non-stl output. Without this
                // the output differs by the last digit of the float
                std::sort(neighbourhood.begin(), neighbourhood.end());

                int cluster = 0;
                float control = 0.0f;

                for (size_t i = 0; i < neighbourhood.size(); i++) {
                    int intersect_size = 0, retro_size = 0;
                    auto &retpt = m_map.getPoint(neighbourhood[i]);
                    if (retpt.filled() && retpt.hasNode()) {
                        retpt.getNode().first();
                        while (!retpt.getNode().is_tail()) {
                            retro_size++;
                            if (std::find(neighbourhood.begin(), neighbourhood.end(),
                                          retpt.getNode().cursor()) != neighbourhood.end()) {
                                intersect_size++;
                            }
                            if (std::find(totalneighbourhood.begin(), totalneighbourhood.end(),
                                          retpt.getNode().cursor()) == totalneighbourhood.end()) {
                                totalneighbourhood.push_back(
                                    retpt.getNode().cursor()); // <- note add does nothing if member
                                                               // already exists
                            }
                            retpt.getNode().next();
                        }
                        control += 1.0f / float(retro_size);
                        cluster += intersect_size;
                    }
                }

                if (neighbourhood.size() > 1) {
                    result.setValue(         //
                        refIdx, cluster_col, //
                        float(cluster /
                              double(neighbourhood.size() * (neighbourhood.size() - 1.0))));
                    result.setValue(         //
                        refIdx, control_col, //
                        float(control));
                    result.setValue(                 //
                        refIdx, controllability_col, //
                        float(double(neighbourhood.size()) / double(totalneighbourhood.size())));
                } else {
                    result.setValue(refIdx, cluster_col, -1);
                    result.setValue(refIdx, control_col, -1);
                    result.setValue(refIdx, controllability_col, -1);
                }
                count++; // <- increment count
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

    result.completed = true;

    return result;
}
