// SPDX-FileCopyrightText: 2019-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaisovistzone.hpp"

#include "../salaprogram.hpp"

AnalysisResult VGAIsovistZone::run(Communicator *) {

    AnalysisResult result;

    auto &attributes = m_map.getAttributeTable();

    if (m_originPointSets.empty()) {
        return result;
    }
    for (const auto &originPointSet : m_originPointSets) {
        std::string originPointSetName = originPointSet.first;

        std::string zoneColumnName = getFormattedColumn( //
            Column::ISOVIST_ZONE_DISTANCE, originPointSetName, m_restrictDistance);
        std::string inverseZoneColumnName = getFormattedColumn( //
            Column::ISOVIST_ZONE_INV_SQ_DISTANCE, originPointSetName, m_restrictDistance);

        const auto &originPoints = originPointSet.second;

        auto zoneColumnIndex = attributes.insertOrResetColumn(zoneColumnName);
        result.addAttribute(zoneColumnName);

        for (const PixelRef ref : originPoints) {
            AttributeRow &row = attributes.getRow(AttributeKey(ref));
            row.setValue(zoneColumnIndex, 0);
            Point &lp = m_map.getPoint(ref);
            std::set<MetricTriple> newPixels;
            extractMetric(lp.getNode(), newPixels, m_map, MetricTriple(0.0f, ref, NoPixel));
            for (auto &zonePixel : newPixels) {
                auto *zonePixelRow = attributes.getRowPtr(AttributeKey(zonePixel.pixel));
                if (zonePixelRow != nullptr) {
                    auto zoneLineDist =
                        static_cast<float>(dist(ref, zonePixel.pixel) * m_map.getSpacing());
                    float currZonePixelVal = zonePixelRow->getValue(zoneColumnIndex);
                    if ((currZonePixelVal == -1 || zoneLineDist < currZonePixelVal) &&
                        (m_restrictDistance <= 0 ||
                         (m_restrictDistance > 0 && zoneLineDist < m_restrictDistance))) {
                        zonePixelRow->setValue(zoneColumnIndex, zoneLineDist);
                    }
                }
            }
        }
        auto inverseZoneColumnIndex = attributes.insertOrResetColumn(inverseZoneColumnName);
        setColumnFormulaAndUpdate(m_map, inverseZoneColumnIndex,
                                  "1/((value(\"" + zoneColumnName + "\") + 1) ^ 2)", std::nullopt);
        result.addAttribute(std::move(inverseZoneColumnName));
    }

    result.completed = true;

    return result;
}

void VGAIsovistZone::extractMetric(Node &n, std::set<MetricTriple> &pixels, PointMap &map,
                                   const MetricTriple &curs) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = n.bin(i);
        for (auto pixVec : bin.pixelVecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.dir) <= pixVec.end().col(bin.dir);) {
                Point &pt = map.getPoint(pix);
                if (pt.filled()) {
                    pixels.insert(MetricTriple(0, pix, curs.pixel));
                }
                pix.move(bin.dir);
            }
        }
    }
}

void VGAIsovistZone::setColumnFormulaAndUpdate(PointMap &pointmap, size_t columnIndex,
                                               std::string formula,
                                               std::optional<const std::set<int>> selectionSet) {
    SalaObj programContext;
    SalaGrf graph;
    graph.map.point = &pointmap;
    programContext = SalaObj(SalaObj::S_POINTMAPOBJ, graph);

    std::istringstream stream(formula);
    SalaProgram proggy(programContext);
    if (!proggy.parse(stream)) {
        throw depthmapX::RuntimeException("There was an error parsing your formula:\n\n" +
                                          proggy.getLastErrorMessage());
    } else {
        bool programCompleted;
        if (selectionSet.has_value()) {
            programCompleted =
                proggy.runupdate(static_cast<int>(columnIndex), selectionSet.value());
        } else {
            programCompleted = proggy.runupdate(static_cast<int>(columnIndex));
        }
        if (!programCompleted) {
            throw depthmapX::RuntimeException("There was an error parsing your formula:\n\n" +
                                              proggy.getLastErrorMessage());
        }
    }
    programContext.getTable()->getColumn(columnIndex).setFormula(std::move(formula));
}
