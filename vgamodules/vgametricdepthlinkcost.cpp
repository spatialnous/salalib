// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricdepthlinkcost.h"

AnalysisResult VGAMetricDepthLinkCost::run(Communicator *) {

    AnalysisResult result;

    AttributeTable &attributes = m_map.getAttributeTable();

    // custom linking costs from the attribute table
    std::string link_metric_cost_col_name = Column::LINK_METRIC_COST;
    int link_metric_cost_col = attributes.getOrInsertColumn(link_metric_cost_col_name);
    result.addAttribute(link_metric_cost_col_name);

    std::string path_length_col_name = Column::METRIC_STEP_DEPTH;
    int path_length_col = attributes.insertOrResetColumn(path_length_col_name);
    result.addAttribute(path_length_col_name);

    depthmapX::ColumnMatrix<MetricPoint> metricPoints(m_map.getRows(), m_map.getCols());

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        MetricPoint &pnt = getMetricPoint(metricPoints, pix);
        pnt.m_point = &(m_map.getPoint(pix));
        if (link_metric_cost_col != -1) {
            float linkCost = row.getRow().getValue(link_metric_cost_col);
            if (linkCost > 0)
                pnt.m_linkCost += linkCost;
        }
    }

    // in order to calculate Penn angle, the MetricPair becomes a metric triple...
    std::set<MetricTriple> search_list; // contains root point

    for (auto &sel : m_pixelsFrom) {
        search_list.insert(MetricTriple(0.0f, sel, NoPixel));
    }

    while (search_list.size()) {
        std::set<MetricTriple>::iterator it = search_list.begin();
        MetricTriple here = *it;
        search_list.erase(it);
        MetricPoint &mp = getMetricPoint(metricPoints, here.pixel);
        // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
        if (mp.m_point->filled() && (mp.m_unseen || (here.dist < mp.m_dist))) {
            extractMetric(mp.m_point->getNode(), metricPoints, search_list, &m_map, here);
            mp.m_dist = here.dist;
            mp.m_unseen = false;
            if (!mp.m_point->getMergePixel().empty()) {
                MetricPoint &mp2 = getMetricPoint(metricPoints, mp.m_point->getMergePixel());
                if (mp2.m_unseen || (here.dist + mp2.m_linkCost < mp2.m_dist)) {
                    mp2.m_dist = here.dist + mp2.m_linkCost;
                    mp2.m_unseen = false;
                    extractMetric(mp2.m_point->getNode(), metricPoints, search_list, &m_map,
                                  MetricTriple(mp2.m_dist, mp.m_point->getMergePixel(), NoPixel));
                }
            }
        }
    }

    for (auto &row : attributes) {
        PixelRef pix = PixelRef(row.getKey().value);
        MetricPoint &pnt = getMetricPoint(metricPoints, pix);
        row.getRow().setValue(path_length_col, float(pnt.m_dist));
    }

    result.completed = true;

    return result;
}

void VGAMetricDepthLinkCost::extractMetric(Node n,
                                           depthmapX::ColumnMatrix<MetricPoint> &metricPoints,
                                           std::set<MetricTriple> &pixels, PointMap *pointdata,
                                           const MetricTriple &curs) {
    MetricPoint &cursMP = getMetricPoint(metricPoints, curs.pixel);
    if (curs.dist == 0.0f || cursMP.m_point->blocked() || pointdata->blockedAdjacent(curs.pixel)) {
        for (int i = 0; i < 32; i++) {
            Bin &bin = n.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start();
                     pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    MetricPoint &mpt = getMetricPoint(metricPoints, pix);
                    // the nullptr check is unfortunately required because somehow depthmap stores
                    // neighbour pixels that are not really filled..
                    if (mpt.m_point != nullptr && mpt.m_unseen &&
                        (mpt.m_dist == -1 ||
                         (curs.dist + pointdata->getSpacing() * dist(pix, curs.pixel) <
                          mpt.m_dist))) {
                        mpt.m_dist =
                            curs.dist + pointdata->getSpacing() * (float)dist(pix, curs.pixel);
                        pixels.insert(MetricTriple(mpt.m_dist, pix, curs.pixel));
                    }
                    pix.move(bin.m_dir);
                }
            }
        }
    }
}
