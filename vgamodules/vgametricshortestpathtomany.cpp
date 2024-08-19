// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgametricshortestpathtomany.h"

AnalysisResult VGAMetricShortestPathToMany::run(Communicator *) {

    auto &attributes = m_map.getAttributeTable();

    // custom linking costs from the attribute table
    std::string linkMetricCostColName = Column::LINK_METRIC_COST;

    std::vector<AnalysisData> analysisData = getAnalysisData(attributes, linkMetricCostColName);
    const auto refs = getRefVector(analysisData);
    const auto graph = getGraph(analysisData, refs, true);

    auto [parents] = traverseFindMany(analysisData, graph, refs, m_pixelsFrom, m_pixelsTo);

    for (const PixelRef &pixelFrom : m_pixelsFrom) {
        analysisData.at(getRefIdx(refs, pixelFrom)).dist = 0.0f;
    }

    std::vector<std::string> colNames;
    { // create all columns
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH, ref));
        }
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH_DISTANCE, ref));
        }
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH_LINKED, ref));
        }
        for (PixelRef ref : m_pixelsTo) {
            colNames.push_back(getFormattedColumn(Column::METRIC_SHORTEST_PATH_ORDER, ref));
        }
    }

    AnalysisResult result(std::move(colNames), attributes.getNumRows());

    std::map<PixelRef, std::vector<int>> columns;
    {
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(
                result.getColumnIndex(getFormattedColumn(Column::METRIC_SHORTEST_PATH, ref)));
        }
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(result.getColumnIndex(
                getFormattedColumn(Column::METRIC_SHORTEST_PATH_DISTANCE, ref)));
        }
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(result.getColumnIndex(
                getFormattedColumn(Column::METRIC_SHORTEST_PATH_LINKED, ref)));
        }
        for (PixelRef ref : m_pixelsTo) {
            columns[ref].push_back(
                result.getColumnIndex(getFormattedColumn(Column::METRIC_SHORTEST_PATH_ORDER, ref)));
        }
    }

    for (PixelRef pixelTo : m_pixelsTo) {
        const std::vector<int> &pixelToCols = columns[pixelTo];
        int pathCol = pixelToCols[0];
        int distCol = pixelToCols[1];
        int linkedCol = pixelToCols[2];
        int orderCol = pixelToCols[3];
        auto pixelToParent = parents.find(pixelTo);
        if (pixelToParent != parents.end()) {

            int counter = 0;
            int linePixelCounter = 0;
            auto *lad = &analysisData.at(getRefIdx(refs, pixelTo));
            result.setValue(lad->attributeDataRow, orderCol, counter);
            result.setValue(lad->attributeDataRow, distCol, lad->dist);

            counter++;
            auto currParent = pixelToParent;
            counter++;
            while (currParent != parents.end()) {
                auto &ad = analysisData.at(getRefIdx(refs, currParent->second));
                auto &p = ad.point;
                result.setValue(ad.attributeDataRow, orderCol, counter);
                result.setValue(ad.attributeDataRow, distCol, ad.dist);

                if (!p.getMergePixel().empty() && p.getMergePixel() == currParent->first) {
                    result.setValue(ad.attributeDataRow, linkedCol, 1);
                    result.setValue(lad->attributeDataRow, linkedCol, 1);
                } else {
                    // apparently we can't just have 1 number in the whole column
                    result.setValue(ad.attributeDataRow, linkedCol, 0);
                    auto pixelated = m_map.quickPixelateLine(currParent->first, currParent->second);
                    for (auto &linePixel : pixelated) {
                        auto *linePixelRow = attributes.getRowPtr(AttributeKey(linePixel));
                        if (linePixelRow != nullptr) {
                            auto &lpad = analysisData.at(getRefIdx(refs, linePixel));
                            result.setValue(lpad.attributeDataRow, pathCol, linePixelCounter++);
                        }
                    }
                }

                lad = &ad;
                currParent = parents.find(currParent->second);
                counter++;
            }
        }
    }
    if (m_pixelsTo.size() > 0) {
        result.completed = true;
    }

    return result;
}
