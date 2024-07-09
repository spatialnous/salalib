// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgathroughvision.h"

#include "salalib/agents/agentanalysis.h"

// This is a slow algorithm, but should give the correct answer
// for demonstrative purposes

AnalysisResult VGAThroughVision::run(Communicator *comm, PointMap &map, bool) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AnalysisResult result;

    AttributeTable &attributes = map.getAttributeTable();

    // current version (not sure of differences!)
    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            map.getPoint(curs).m_misc = 0;
        }
    }

    auto agentGateColIdx =
        map.getAttributeTable().getColumnIndexOptional(AgentAnalysis::Column::INTERNAL_GATE);
    auto agentGateCountColIdx =
        map.getAttributeTable().getColumnIndexOptional(AgentAnalysis::Column::INTERNAL_GATE_COUNTS);

    int count = 0;
    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            std::vector<int> seengates;
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            Point &p = map.getPoint(curs);
            if (map.getPoint(curs).filled()) {
                p.getNode().first();
                while (!p.getNode().is_tail()) {
                    PixelRef x = p.getNode().cursor();
                    PixelRefVector pixels = map.quickPixelateLine(x, curs);
                    for (size_t k = 1; k < pixels.size() - 1; k++) {
                        PixelRef key = pixels[k];
                        map.getPoint(key).m_misc += 1;

                        // TODO: Undocumented functionality. Shows how many times a gate is passed?
                        if (agentGateColIdx.has_value() && agentGateCountColIdx.has_value()) {
                            auto iter = attributes.find(AttributeKey(key));
                            if (iter != attributes.end()) {
                                int gate = static_cast<int>(
                                    iter->getRow().getValue(agentGateColIdx.value()));
                                if (gate != -1) {
                                    auto gateIter = depthmapX::findBinary(seengates, gate);
                                    if (gateIter == seengates.end()) {
                                        iter->getRow().incrValue(agentGateCountColIdx.has_value());
                                        seengates.insert(gateIter, int(gate));
                                    }
                                }
                            }
                        }
                    }
                    p.getNode().next();
                }
                // only increment count for actual filled points
                count++;
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

    int col = attributes.getOrInsertColumn(Column::THROUGH_VISION);
    result.addAttribute(Column::THROUGH_VISION);

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        PixelRef pix = iter->getKey().value;
        iter->getRow().setValue(col, static_cast<float>(map.getPoint(pix).m_misc));
        map.getPoint(pix).m_misc = 0;
    }

    result.completed = true;

    return result;
}
