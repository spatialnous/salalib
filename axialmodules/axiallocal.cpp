// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "axiallocal.h"

AnalysisResult AxialLocal::run(Communicator *comm, ShapeGraph &map, bool) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getShapeCount());
    }

    AnalysisResult result;

    AttributeTable &attributes = map.getAttributeTable();

    attributes.insertOrResetColumn("Control");
    result.addAttribute("Control");
    attributes.insertOrResetColumn("Controllability");
    result.addAttribute("Controllability");

    size_t control_col = attributes.getColumnIndex("Control");
    size_t controllability_col = attributes.getColumnIndex("Controllability");

    // n.b., for this operation we assume continuous line referencing from zero (this is silly?)
    // has already failed due to this!  when intro hand drawn fewest line (where user may have
    // deleted) it's going to get worse...

    size_t i = 0;
    for (auto &iter : attributes) {
        AttributeRow &row = iter.getRow();

        double control = 0.0;
        const auto &connections = map.getConnections()[i].m_connections;
        std::vector<size_t> totalneighbourhood;
        for (auto connection : connections) {
            // n.b., as of Depthmap 10.0, connections[j] and i cannot coexist
            // if (connections[j] != i) {
            depthmapX::addIfNotExists(totalneighbourhood, connection);
            int retro_size = 0;
            auto &retconnectors = map.getConnections()[connection].m_connections;
            for (auto retconnector : retconnectors) {
                retro_size++;
                depthmapX::addIfNotExists(totalneighbourhood, retconnector);
            }
            control += 1.0 / double(retro_size);
            //}
        }

        if (connections.size() > 0) {
            row.setValue(control_col, float(control));
            row.setValue(controllability_col,
                         float(double(connections.size()) / double(totalneighbourhood.size() - 1)));
        } else {
            row.setValue(control_col, -1);
            row.setValue(controllability_col, -1);
        }

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
        i++;
    }

    result.completed = true;

    return result;
}
