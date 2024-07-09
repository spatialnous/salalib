// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangulardepth.h"

AnalysisResult VGAAngularDepth::run(Communicator *, PointMap &map, bool) {

    AnalysisResult result;

    AttributeTable &attributes = map.getAttributeTable();

    // n.b., insert columns sets values to -1 if the column already exists
    auto sdColIdx = attributes.insertOrResetColumn(Column::ANGULAR_STEP_DEPTH);
    result.addAttribute(Column::ANGULAR_STEP_DEPTH);

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        PixelRef pix = iter->getKey().value;
        map.getPoint(pix).m_misc = 0;
        map.getPoint(pix).m_dist = 0.0f;
        map.getPoint(pix).m_cumangle = -1.0f;
    }

    std::set<AngularTriple> search_list; // contains root point

    for (auto &sel : m_originRefs) {
        search_list.insert(AngularTriple(0.0f, sel, NoPixel));
        map.getPoint(sel).m_cumangle = 0.0f;
    }

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    while (search_list.size()) {
        std::set<AngularTriple>::iterator it = search_list.begin();
        AngularTriple here = *it;
        search_list.erase(it);
        Point &p = map.getPoint(here.pixel);
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (p.filled() && p.m_misc != ~0) {
            p.getNode().extractAngular(search_list, &map, here);
            p.m_misc = ~0;
            AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(here.pixel));
            row.setValue(sdColIdx, float(p.m_cumangle));
            if (!p.getMergePixel().empty()) {
                Point &p2 = map.getPoint(p.getMergePixel());
                if (p2.m_misc != ~0) {
                    p2.m_cumangle = p.m_cumangle;
                    AttributeRow &mergePixelRow =
                        map.getAttributeTable().getRow(AttributeKey(p.getMergePixel()));
                    mergePixelRow.setValue(sdColIdx, float(p2.m_cumangle));
                    p2.getNode().extractAngular(
                        search_list, &map, AngularTriple(here.angle, p.getMergePixel(), NoPixel));
                    p2.m_misc = ~0;
                }
            }
        }
    }

    result.completed = true;

    return result;
}
