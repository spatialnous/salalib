// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extractlinkdata.h"

AnalysisResult ExtractLinkData::run(Communicator *) {

    AnalysisResult result;

    auto &attributes = m_map.getAttributeTable();

    std::string angular_cost_col_name = Column::LINK_ANGULAR_COST;
    int angular_cost_col = attributes.insertOrResetColumn(angular_cost_col_name);
    result.addAttribute(angular_cost_col_name);
    std::string metric_cost_col_name = Column::LINK_METRIC_COST;
    int metric_cost_col = attributes.insertOrResetColumn(metric_cost_col_name);
    result.addAttribute(metric_cost_col_name);
    std::string link_to_col_name = Column::LINK_TO;
    int link_to_col = attributes.insertOrResetColumn(link_to_col_name);
    result.addAttribute(link_to_col_name);
    std::string visual_cost_col_name = Column::LINK_VISUAL_COST;
    int visual_cost_col = attributes.insertOrResetColumn(visual_cost_col_name);
    result.addAttribute(visual_cost_col_name);

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        Point &p = m_map.getPoint(pix);
        PixelRef mergePixel = p.getMergePixel();
        if (!mergePixel.empty()) {
            row.getRow().setValue(link_to_col, static_cast<int>(mergePixel));
            row.getRow().setValue(visual_cost_col, 1);
            row.getRow().setValue(metric_cost_col, dist(pix, mergePixel) * m_map.getSpacing());
            row.getRow().setValue(angular_cost_col, 1);
        }
    }

    result.completed = true;

    return result;
}
