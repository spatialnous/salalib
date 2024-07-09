// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaangularopenmp.h"

#include "genlib/stringutils.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

AnalysisResult VGAAngularOpenMP::run(Communicator *comm) {

#if !defined(_OPENMP)
    std::cerr << "OpenMP NOT available, only running on a single core" << std::endl;
#endif

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    AttributeTable &attributes = m_map.getAttributeTable();

    std::vector<PixelRef> filled;
    std::vector<AttributeRow *> rows;

    for (size_t i = 0; i < m_map.getCols(); i++) {
        for (size_t j = 0; j < m_map.getRows(); j++) {
            PixelRef curs = PixelRef(static_cast<short>(i), static_cast<short>(j));
            if (m_map.getPoint(curs).filled()) {
                filled.push_back(curs);
                rows.push_back(attributes.getRowPtr(AttributeKey(curs)));
            }
        }
    }

    int count = 0;

    std::vector<DataPoint> col_data(filled.size());

    int i, N = int(filled.size());
#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif
    for (i = 0; i < N; i++) {
        if (m_gates_only) {
            count++;
            continue;
        }

        DataPoint &dp = col_data[i];

        depthmapX::RowMatrix<int> miscs(m_map.getRows(), m_map.getCols());
        depthmapX::RowMatrix<float> cumangles(m_map.getRows(), m_map.getCols());

        miscs.initialiseValues(0);
        cumangles.initialiseValues(-1.0f);

        float total_angle = 0.0f;
        int total_nodes = 0;

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only

        std::set<AngularTriple> search_list;
        search_list.insert(AngularTriple(0.0f, filled[size_t(i)], NoPixel));
        cumangles(filled[size_t(i)].y, filled[size_t(i)].x) = 0.0f;

        while (search_list.size()) {
            std::set<AngularTriple>::iterator it = search_list.begin();
            AngularTriple here = *it;
            search_list.erase(it);
            if (int(m_radius) != -1 && double(here.angle) > m_radius) {
                break;
            }
            Point &p = m_map.getPoint(here.pixel);
            int &p1misc = miscs(here.pixel.y, here.pixel.x);
            float &p1cumangle = cumangles(here.pixel.y, here.pixel.x);
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && p1misc != ~0) {
                extractAngular(p.getNode(), search_list, &m_map, here, miscs, cumangles);
                p1misc = ~0;
                if (!p.getMergePixel().empty()) {
                    Point &p2 = m_map.getPoint(p.getMergePixel());
                    int &p2misc = miscs(p.getMergePixel().y, p.getMergePixel().x);
                    float &p2cumangle = cumangles(p.getMergePixel().y, p.getMergePixel().x);
                    if (p2misc != ~0) {
                        p2cumangle = p1cumangle;
                        extractAngular(p2.getNode(), search_list, &m_map,
                                       AngularTriple(here.angle, p.getMergePixel(), NoPixel), miscs,
                                       cumangles);
                        p2misc = ~0;
                    }
                }
                total_angle += p1cumangle;
                total_nodes += 1;
            }
        }

        if (total_nodes > 0) {
            dp.mean_depth = float(double(total_angle) / double(total_nodes));
        }
        dp.total_depth = total_angle;
        dp.count = float(total_nodes);

        count++; // <- increment count

        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
            }
        }

        // kept to achieve parity in binary comparison with old versions
        // TODO: Remove at next version of .graph file
        m_map.getPoint(filled[size_t(i)]).m_misc = miscs(filled[size_t(i)].y, filled[size_t(i)].x);
        m_map.getPoint(filled[size_t(i)]).m_cumangle =
            cumangles(filled[size_t(i)].y, filled[size_t(i)].x);
    }

    AnalysisResult result;

    // n.b. these must be entered in alphabetical order to preserve col indexing:
    std::string mean_depth_col_text =
        getColumnWithRadius(Column::ANGULAR_MEAN_DEPTH, m_radius, m_map.getRegion());
    int mean_depth_col = attributes.getOrInsertColumn(mean_depth_col_text.c_str());
    result.addAttribute(mean_depth_col_text);
    std::string total_detph_col_text =
        getColumnWithRadius(Column::ANGULAR_TOTAL_DEPTH, m_radius, m_map.getRegion());
    int total_depth_col = attributes.getOrInsertColumn(total_detph_col_text.c_str());
    result.addAttribute(total_detph_col_text);
    std::string count_col_text =
        getColumnWithRadius(Column::ANGULAR_NODE_COUNT, m_radius, m_map.getRegion());
    int count_col = attributes.getOrInsertColumn(count_col_text.c_str());
    result.addAttribute(count_col_text);

    auto dataIter = col_data.begin();
    for (auto row : rows) {
        row->setValue(mean_depth_col, dataIter->mean_depth);
        row->setValue(total_depth_col, dataIter->total_depth);
        row->setValue(count_col, dataIter->count);
        dataIter++;
    }

    result.completed = true;

    return result;
}

void VGAAngularOpenMP::extractAngular(Node &node, std::set<AngularTriple> &pixels,
                                      PointMap *pointdata, const AngularTriple &curs,
                                      depthmapX::RowMatrix<int> &miscs,
                                      depthmapX::RowMatrix<float> &cumangles) {
    if (curs.angle == 0.0f || pointdata->getPoint(curs.pixel).blocked() ||
        pointdata->blockedAdjacent(curs.pixel)) {
        for (int i = 0; i < 32; i++) {
            Bin &bin = node.bin(i);
            for (auto pixVec : bin.m_pixel_vecs) {
                for (PixelRef pix = pixVec.start();
                     pix.col(bin.m_dir) <= pixVec.end().col(bin.m_dir);) {
                    if (miscs(pix.y, pix.x) == 0) {
                        // n.b. dmap v4.06r now sets angle in range 0 to 4 (1 = 90 degrees)
                        float ang =
                            (curs.lastpixel == NoPixel)
                                ? 0.0f
                                : (float)(angle(pix, curs.pixel, curs.lastpixel) / (M_PI * 0.5));
                        float &cumangle = cumangles(pix.y, pix.x);
                        if (cumangle == -1.0 || curs.angle + ang < cumangle) {
                            cumangle = cumangles(curs.pixel.y, curs.pixel.x) + ang;
                            pixels.insert(AngularTriple(cumangle, pix, curs.pixel));
                        }
                    }
                    pix.move(bin.m_dir);
                }
            }
        }
    }
}
