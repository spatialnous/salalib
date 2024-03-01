// sala - a component of the depthmapX - spatial network analysis platform
// Copyright (C) 2000-2010, University College London, Alasdair Turner
// Copyright (C) 2011-2012, Tasos Varoudis
// Copyright (C) 2017-2024, Petros Koutsolampros

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "salalib/axialmodules/axiallocal.h"

AnalysisResult AxialLocal::run(Communicator *comm,
                               ShapeGraph &map,
                               bool simple_version) {
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


    int control_col = -1, controllability_col = -1;
    control_col = attributes.getColumnIndex("Control");
    controllability_col = attributes.getColumnIndex("Controllability");


    // n.b., for this operation we assume continuous line referencing from zero (this is silly?)
    // has already failed due to this!  when intro hand drawn fewest line (where user may have
    // deleted) it's going to get worse...


    size_t i = -1;
    for (auto &iter : attributes) {
        i++;
        AttributeRow &row = iter.getRow();

        double control = 0.0;
        const std::vector<int> &connections = map.getConnections()[i].m_connections;
        std::vector<int> totalneighbourhood;
        for (int connection : connections) {
            // n.b., as of Depthmap 10.0, connections[j] and i cannot coexist
            // if (connections[j] != i) {
            depthmapX::addIfNotExists(totalneighbourhood, connection);
            int retro_size = 0;
            auto &retconnectors = map.getConnections()[size_t(connection)].m_connections;
            for (auto retconnector : retconnectors) {
                retro_size++;
                depthmapX::addIfNotExists(totalneighbourhood, retconnector);
            }
            control += 1.0 / double(retro_size);
            //}
        }

        if (connections.size() > 0) {
            row.setValue(control_col, float(control));
            row.setValue(controllability_col, float(double(connections.size()) /
                                                    double(totalneighbourhood.size() - 1)));
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
    }

    result.completed = true;

    return result;
}
