// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extractlinkdata.h"

AnalysisResult ExtractLinkData::run(Communicator *) {

    AnalysisResult result;

    auto &attributes = m_map.getAttributeTable();

    std::string angularCostColName = Column::LINK_ANGULAR_COST;
    auto angularCostCol = attributes.insertOrResetColumn(angularCostColName);
    result.addAttribute(angularCostColName);
    std::string metricCostColName = Column::LINK_METRIC_COST;
    auto metricCostCol = attributes.insertOrResetColumn(metricCostColName);
    result.addAttribute(metricCostColName);
    std::string linkToColName = Column::LINK_TO;
    auto linkToCol = attributes.insertOrResetColumn(linkToColName);
    result.addAttribute(linkToColName);
    std::string visualCostColName = Column::LINK_VISUAL_COST;
    auto visualCostCol = attributes.insertOrResetColumn(visualCostColName);
    result.addAttribute(visualCostColName);

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        Point &p = m_map.getPoint(pix);
        PixelRef mergePixel = p.getMergePixel();
        if (!mergePixel.empty()) {
            row.getRow().setValue(linkToCol, static_cast<float>(mergePixel));
            row.getRow().setValue(visualCostCol, 1);
            row.getRow().setValue(metricCostCol,
                                  static_cast<float>(dist(pix, mergePixel) * m_map.getSpacing()));
            row.getRow().setValue(angularCostCol, 1);
        }
    }

    result.completed = true;

    return result;
}
