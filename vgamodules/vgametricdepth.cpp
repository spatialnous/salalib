// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricdepth.h"

AnalysisResult VGAMetricDepth::run(Communicator *, PointMap &map, bool) {

    AnalysisResult result;

    AttributeTable &attributes = map.getAttributeTable();

    // n.b., insert columns sets values to -1 if the column already exists
    auto path_angle_col = attributes.insertOrResetColumn(Column::METRIC_STEP_SHORTEST_PATH_ANGLE);
    result.addAttribute(Column::METRIC_STEP_SHORTEST_PATH_ANGLE);
    auto path_length_col = attributes.insertOrResetColumn(Column::METRIC_STEP_SHORTEST_PATH_LENGTH);
    result.addAttribute(Column::METRIC_STEP_SHORTEST_PATH_LENGTH);
    std::optional<size_t> dist_col = std::nullopt;
    if (m_originRefs.size() == 1) {
        // Note: Euclidean distance is currently only calculated from a single point
        dist_col = attributes.insertOrResetColumn(Column::METRIC_STRAIGHT_LINE_DISTANCE);
        result.addAttribute(Column::METRIC_STRAIGHT_LINE_DISTANCE);
    }

    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        PixelRef pix = iter->getKey().value;
        map.getPoint(pix).m_misc = 0;
        map.getPoint(pix).m_dist = -1.0f;
        map.getPoint(pix).m_cumangle = 0.0f;
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<MetricTriple> search_list; // contains root point

    for (auto &sel : m_originRefs) {
        search_list.insert(MetricTriple(0.0f, sel, NoPixel));
    }

    // note that m_misc is used in a different manner to analyseGraph / PointDepth
    // here it marks the node as used in calculation only
    while (search_list.size()) {
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        Point &p = map.getPoint(here.pixel);
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (p.filled() && p.m_misc != ~0) {
            p.getNode().extractMetric(search_list, &map, here);
            p.m_misc = ~0;
            AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(here.pixel));
            row.setValue(path_length_col, float(map.getSpacing() * here.dist));
            row.setValue(path_angle_col, float(p.m_cumangle));
            if (m_originRefs.size() == 1) {
                // Note: Euclidean distance is currently only calculated from a single point
                row.setValue(dist_col.value(),
                             float(map.getSpacing() * dist(here.pixel, *m_originRefs.begin())));
            }
            if (!p.getMergePixel().empty()) {
                Point &p2 = map.getPoint(p.getMergePixel());
                if (p2.m_misc != ~0) {
                    p2.m_cumangle = p.m_cumangle;
                    AttributeRow &mergePixelRow =
                        map.getAttributeTable().getRow(AttributeKey(p.getMergePixel()));
                    mergePixelRow.setValue(path_length_col, float(map.getSpacing() * here.dist));
                    mergePixelRow.setValue(path_angle_col, float(p2.m_cumangle));
                    if (m_originRefs.size() == 1) {
                        // Note: Euclidean distance is currently only calculated from a single point
                        mergePixelRow.setValue(
                            dist_col.value(),
                            float(map.getSpacing() *
                                  dist(p.getMergePixel(), *m_originRefs.begin())));
                    }
                    p2.getNode().extractMetric(search_list, &map,
                                               MetricTriple(here.dist, p.getMergePixel(), NoPixel));
                    p2.m_misc = ~0;
                }
            }
        }
    }

    result.completed = true;

    return result;
}
