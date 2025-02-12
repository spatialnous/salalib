// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgathroughvision.hpp"

#include "../agents/agentanalysis.hpp"

// This is a slow algorithm, but should give the correct answer
// for demonstrative purposes

AnalysisResult VGAThroughVision::run(Communicator *comm) {
    auto &attributes = m_map.getAttributeTable();

    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS,
                              static_cast<size_t>(m_map.getFilledPointCount()));
    }

    std::vector<AnalysisData> analysisData;
    analysisData.reserve(attributes.getNumRows());

    size_t rowCounter = 0;
    for (auto &attRow : attributes) {
        auto &point = m_map.getPoint(attRow.getKey().value);
        analysisData.push_back(AnalysisData(point, attRow.getKey().value, rowCounter, 0));
        rowCounter++;
    }

    auto agentGateColIdx =
        m_map.getAttributeTable().getColumnIndexOptional(AgentAnalysis::Column::INTERNAL_GATE);
    auto agentGateCountColIdx = m_map.getAttributeTable().getColumnIndexOptional(
        AgentAnalysis::Column::INTERNAL_GATE_COUNTS);

    std::vector<std::string> cols = {Column::THROUGH_VISION};
    if (agentGateColIdx.has_value() && agentGateCountColIdx.has_value()) {
        cols.push_back(AgentAnalysis::Column::INTERNAL_GATE_COUNTS);
    }
    AnalysisResult result(std::move(cols), attributes.getNumRows());

    const auto refs = getRefVector(analysisData);

    size_t count = 0;
    for (auto &ad : analysisData) {
        std::vector<int> seengates;
        auto &p = ad.point;
        p.getNode().first();
        while (!p.getNode().is_tail()) {
            PixelRef x = p.getNode().cursor();
            PixelRefVector pixels = m_map.quickPixelateLine(x, ad.ref);
            for (size_t k = 1; k < pixels.size() - 1; k++) {
                PixelRef key = pixels[k];
                if (!m_map.getPoint(key).filled())
                    continue;
                analysisData.at(getRefIdx(refs, key)).misc += 1;

                // TODO: Undocumented functionality. Shows how many times a gate is passed?
                if (agentGateColIdx.has_value() && agentGateCountColIdx.has_value()) {
                    auto iter = attributes.find(AttributeKey(key));
                    if (iter != m_map.getAttributeTable().end()) {
                        int gate =
                            static_cast<int>(iter->getRow().getValue(agentGateColIdx.value()));
                        if (gate != -1) {
                            auto gateIter = depthmapX::findBinary(seengates, gate);
                            if (gateIter == seengates.end()) {
                                result.incrValue(ad.attributeDataRow, agentGateCountColIdx.value());
                                seengates.insert(gateIter, static_cast<int>(gate));
                            }
                        }
                    }
                }
            }
            p.getNode().next();
        }
        // only increment count for actual filled points
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

    auto col = result.getColumnIndex(Column::THROUGH_VISION);

    for (auto &ad : analysisData) {
        auto &p = ad.point;
        result.setValue(ad.attributeDataRow, col, static_cast<float>(ad.misc));
        p.dummyMisc = 0;
    }

    result.completed = true;

    return result;
}
