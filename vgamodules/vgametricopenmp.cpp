// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricopenmp.h"

#if defined(_OPENMP)
#include <omp.h>
#endif

AnalysisResult VGAMetricOpenMP::run(Communicator *comm) {

#if !defined(_OPENMP)
    std::cerr << "OpenMP NOT available, only running on a single core" << std::endl;
    m_forceCommUpdatesMasterThread = false;
#else
    if (m_limitToThreads.has_value()) {
        omp_set_num_threads(m_limitToThreads.value());
    }
#endif

    auto &attributes = m_map.getAttributeTable();

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, m_map.getFilledPointCount());
    }

    std::vector<AnalysisData> globalAnalysisData;
    globalAnalysisData.reserve(m_map.getAttributeTable().getNumRows());

    size_t rowCounter = 0;
    for (auto &attRow : attributes) {
        auto &point = m_map.getPoint(attRow.getKey().value);
        globalAnalysisData.push_back(AnalysisData(point, attRow.getKey().value, rowCounter, 0,
                                                  attRow.getKey().value, -1.0f, 0.0f));
        rowCounter++;
    }

    const auto refs = getRefVector(globalAnalysisData);
    const auto graph = getGraph(globalAnalysisData, refs, false);

    int count = 0;

    std::vector<DataPoint> col_data(attributes.getNumRows());

    int i, N = int(attributes.getNumRows());

#if defined(_OPENMP)
#pragma omp parallel for default(shared) private(i) schedule(dynamic)
#endif

    for (i = 0; i < N; i++) {
        if (m_gates_only) {
#if defined(_OPENMP)
#pragma omp critical
#endif
            count++;
            continue;
        }

        DataPoint &dp = col_data[i];

        std::vector<AnalysisData> analysisData;
        analysisData.reserve(m_map.getAttributeTable().getNumRows());

        size_t rowCounter = 0;
        for (auto &attRow : attributes) {
            auto &point = m_map.getPoint(attRow.getKey().value);
            analysisData.push_back(AnalysisData(point, attRow.getKey().value, rowCounter, 0,
                                                attRow.getKey().value, -1.0f, 0.0f));
            rowCounter++;
        }

        float euclid_depth = 0.0f;
        float total_depth = 0.0f;
        float total_angle = 0.0f;
        int total_nodes = 0;

        // note that m_misc is used in a different manner to analyseGraph / PointDepth
        // here it marks the node as used in calculation only

        std::set<MetricSearchData> search_list;
        auto &ad0 = analysisData.at(i);
        search_list.insert(MetricSearchData(ad0, 0.0f, std::nullopt));
        while (search_list.size()) {
            auto internalNode = search_list.extract(search_list.begin());
            MetricSearchData here = std::move(internalNode.value());

            if (int(m_radius) != -1 && double(here.dist) * m_map.getSpacing() > m_radius) {
                break;
            }
            auto &ad1 = here.pixel;
            auto &p = here.pixel.m_point;
            // nb, the filled check is necessary as diagonals seem to be stored with 'gaps' left in
            if (p.filled() && here.pixel.m_visitedFromBin != ~0) {
                extractMetric(graph.at(ad1.m_attributeDataRow), search_list, m_map, here);
                here.pixel.m_visitedFromBin = ~0;
                if (!p.getMergePixel().empty()) {
                    auto &ad2 = analysisData.at(getRefIdx(refs, p.getMergePixel()));
                    if (ad2.m_visitedFromBin != ~0) {
                        ad2.m_cumAngle = here.pixel.m_cumAngle;
                        extractMetric(graph.at(ad2.m_attributeDataRow), search_list, m_map,
                                      MetricSearchData(ad2, here.dist, std::nullopt));
                        ad2.m_visitedFromBin = ~0;
                    }
                }
                total_depth += here.dist * float(m_map.getSpacing());
                total_angle += here.pixel.m_cumAngle;
                euclid_depth += float(m_map.getSpacing() * dist(here.pixel.m_ref, ad0.m_ref));
                total_nodes += 1;
            }
        }

        if (m_legacyWriteMiscs) {
            // kept to achieve parity in binary comparison with old versions
            ad0.m_point.m_dummy_misc = ad0.m_visitedFromBin;
            ad0.m_point.m_dummy_dist = ad0.m_dist;
            ad0.m_point.m_dummy_cumangle = ad0.m_cumAngle;
        }

        dp.mspa = float(double(total_angle) / double(total_nodes));
        dp.mspl = float(double(total_depth) / double(total_nodes));
        dp.dist = float(double(euclid_depth) / double(total_nodes));
        dp.count = float(total_nodes);

#if defined(_OPENMP)
#pragma omp atomic
#endif
        count++; // <- increment count

#if defined(_OPENMP)
        // only executed by the main thread if requested
        if (!m_forceCommUpdatesMasterThread || omp_get_thread_num() == 0)
#endif
            if (comm) {
                if (qtimer(atime, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
                }
            }
    }

    std::string mspa_col_text = getColumnWithRadius(Column::METRIC_MEAN_SHORTEST_PATH_ANGLE,    //
                                                    m_radius, m_map.getRegion());               //
    std::string mspl_col_text = getColumnWithRadius(Column::METRIC_MEAN_SHORTEST_PATH_DISTANCE, //
                                                    m_radius, m_map.getRegion());               //
    std::string dist_col_text = getColumnWithRadius(Column::METRIC_MEAN_STRAIGHT_LINE_DISTANCE, //
                                                    m_radius, m_map.getRegion());               //
    std::string count_col_text = getColumnWithRadius(Column::METRIC_NODE_COUNT,                 //
                                                     m_radius, m_map.getRegion());              //

    AnalysisResult result({mspa_col_text, mspl_col_text, dist_col_text, count_col_text},
                          attributes.getNumRows());

    int mspa_col = result.getColumnIndex(mspa_col_text);
    int mspl_col = result.getColumnIndex(mspl_col_text);
    int dist_col = result.getColumnIndex(dist_col_text);
    int count_col = result.getColumnIndex(count_col_text);

    auto dataIter = col_data.begin();
    for (size_t i = 0; i < attributes.getNumRows(); i++) {
        result.setValue(i, mspa_col, dataIter->mspa);
        result.setValue(i, mspl_col, dataIter->mspl);
        result.setValue(i, dist_col, dataIter->dist);
        result.setValue(i, count_col, dataIter->count);
        dataIter++;
    }

    result.completed = true;

    return result;
}
