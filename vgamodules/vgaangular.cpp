// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangular.h"

#include "genlib/stringutils.h"

AnalysisResult VGAAngular::run(Communicator *comm, PointMap &map, bool) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, map.getFilledPointCount());
    }

    AnalysisResult result;

    std::string radius_text;
    if (m_radius != -1.0) {
        if (map.getRegion().width() > 100.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.f");
        } else if (map.getRegion().width() < 1.0) {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.4f");
        } else {
            radius_text = std::string(" R") + dXstring::formatString(m_radius, "%.2f");
        }
    }

    AttributeTable &attributes = map.getAttributeTable();

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    std::string mean_depth_col_text = std::string("Angular Mean Depth") + radius_text;
    int mean_depth_col = attributes.getOrInsertColumn(mean_depth_col_text.c_str());
    result.addAttribute(mean_depth_col_text);
    std::string total_detph_col_text = std::string("Angular Total Depth") + radius_text;
    int total_depth_col = attributes.getOrInsertColumn(total_detph_col_text.c_str());
    result.addAttribute(total_detph_col_text);
    std::string count_col_text = std::string("Angular Node Count") + radius_text;
    int count_col = attributes.getOrInsertColumn(count_col_text.c_str());
    result.addAttribute(count_col_text);

    // TODO: Binary compatibility. Remove in re-examination
    total_depth_col = attributes.getOrInsertColumn(total_detph_col_text.c_str());
    result.addAttribute(total_detph_col_text);

    int count = 0;

    for (size_t i = 0; i < map.getCols(); i++) {
        for (size_t j = 0; j < map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));

            if (map.getPoint(curs).filled()) {

                if (m_gates_only) {
                    count++;
                    continue;
                }

                // TODO: Break out miscs/dist/cumangle into local variables and remove from Point
                // class
                for (auto &point : map.getPoints()) {
                    point.m_misc = 0;
                    point.m_dist = 0.0f;
                    point.m_cumangle = -1.0f;
                }

                float total_angle = 0.0f;
                int total_nodes = 0;

                // note that m_misc is used in a different manner to analyseGraph / PointDepth
                // here it marks the node as used in calculation only

                std::set<AngularTriple> search_list;
                search_list.insert(AngularTriple(0.0f, curs, NoPixel));
                map.getPoint(curs).m_cumangle = 0.0f;
                while (search_list.size()) {
                    std::set<AngularTriple>::iterator it = search_list.begin();
                    AngularTriple here = *it;
                    search_list.erase(it);
                    if (m_radius != -1.0 && here.angle > m_radius) {
                        break;
                    }
                    Point &p = map.getPoint(here.pixel);
                    // nb, the filled check is necessary as diagonals seem to be stored with 'gaps'
                    // left in
                    if (p.filled() && p.m_misc != ~0) {
                        p.getNode().extractAngular(search_list, &map, here);
                        p.m_misc = ~0;
                        if (!p.getMergePixel().empty()) {
                            Point &p2 = map.getPoint(p.getMergePixel());
                            if (p2.m_misc != ~0) {
                                p2.m_cumangle = p.m_cumangle;
                                p2.getNode().extractAngular(
                                    search_list, &map,
                                    AngularTriple(here.angle, p.getMergePixel(), NoPixel));
                                p2.m_misc = ~0;
                            }
                        }
                        total_angle += p.m_cumangle;
                        total_nodes += 1;
                    }
                }

                AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(curs));
                if (total_nodes > 0) {
                    row.setValue(mean_depth_col, float(double(total_angle) / double(total_nodes)));
                }
                row.setValue(total_depth_col, total_angle);
                row.setValue(count_col, float(total_nodes));

                count++; // <- increment count
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

    map.setDisplayedAttribute(-2);
    map.setDisplayedAttribute(mean_depth_col);

    result.completed = true;

    return result;
}
