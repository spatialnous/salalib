// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgavisuallocal.h"

AnalysisResult VGAVisualLocal::run(Communicator *comm, PointMap &map, bool simple_version) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AnalysisResult result;

    std::optional<size_t> cluster_col = std::nullopt, control_col = std::nullopt,
                          controllability_col = std::nullopt;
    if (!simple_version) {
        cluster_col =
            map.getAttributeTable().insertOrResetColumn(Column::VISUAL_CLUSTERING_COEFFICIENT);
        result.addAttribute(Column::VISUAL_CLUSTERING_COEFFICIENT);
        control_col = map.getAttributeTable().insertOrResetColumn(Column::VISUAL_CONTROL);
        result.addAttribute(Column::VISUAL_CONTROL);
        controllability_col =
            map.getAttributeTable().insertOrResetColumn(Column::VISUAL_CONTROLLABILITY);
        result.addAttribute(Column::VISUAL_CONTROLLABILITY);
    }

    size_t count = 0;

    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (map.getPoint(curs).filled()) {
                if ((map.getPoint(curs).contextfilled() && !curs.iseven()) || (m_gates_only)) {
                    count++;
                    continue;
                }
                AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(curs));

                // This is much easier to do with a straight forward list:
                PixelRefVector neighbourhood;
                PixelRefVector totalneighbourhood;
                map.getPoint(curs).getNode().contents(neighbourhood);

                // only required to match previous non-stl output. Without this
                // the output differs by the last digit of the float
                std::sort(neighbourhood.begin(), neighbourhood.end());

                int cluster = 0;
                float control = 0.0f;

                for (size_t i = 0; i < neighbourhood.size(); i++) {
                    int intersect_size = 0, retro_size = 0;
                    Point &retpt = map.getPoint(neighbourhood[i]);
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
#ifndef _COMPILE_dX_SIMPLE_VERSION
                if (!simple_version) {
                    if (neighbourhood.size() > 1) {
                        row.setValue(cluster_col.value(),
                                     float(cluster / double(neighbourhood.size() *
                                                            (neighbourhood.size() - 1.0))));
                        row.setValue(control_col.value(), float(control));
                        row.setValue(controllability_col.value(),
                                     float(double(neighbourhood.size()) /
                                           double(totalneighbourhood.size())));
                    } else {
                        row.setValue(cluster_col.value(), -1);
                        row.setValue(control_col.value(), -1);
                        row.setValue(controllability_col.value(), -1);
                    }
                }
#endif
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
