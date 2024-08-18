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

    attributes.insertOrResetColumn(Column::CONTROL);
    result.addAttribute(Column::CONTROL);
    attributes.insertOrResetColumn(Column::CONTROLLABILITY);
    result.addAttribute(Column::CONTROLLABILITY);

    size_t controlCol = attributes.getColumnIndex(Column::CONTROL);
    size_t controllabilityCol = attributes.getColumnIndex(Column::CONTROLLABILITY);

    // n.b., for this operation we assume continuous line referencing from zero (this is silly?)
    // has already failed due to this!  when intro hand drawn fewest line (where user may have
    // deleted) it's going to get worse...

    size_t i = 0;
    for (auto &iter : attributes) {
        AttributeRow &row = iter.getRow();

        double control = 0.0;
        const auto &connections = map.getConnections()[i].connections;
        std::vector<size_t> totalneighbourhood;
        for (auto connection : connections) {
            // n.b., as of Depthmap 10.0, connections[j] and i cannot coexist
            // if (connections[j] != i) {
            depthmapX::addIfNotExists(totalneighbourhood, connection);
            int retroSize = 0;
            auto &retconnectors = map.getConnections()[connection].connections;
            for (auto retconnector : retconnectors) {
                retroSize++;
                depthmapX::addIfNotExists(totalneighbourhood, retconnector);
            }
            control += 1.0 / double(retroSize);
            //}
        }

        if (connections.size() > 0) {
            row.setValue(controlCol, float(control));
            row.setValue(controllabilityCol,
                         float(double(connections.size()) / double(totalneighbourhood.size() - 1)));
        } else {
            row.setValue(controlCol, -1);
            row.setValue(controllabilityCol, -1);
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
