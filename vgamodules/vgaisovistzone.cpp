// SPDX-FileCopyrightText: 2019-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vgaisovistzone.h"

#include "salalib/salaprogram.h"

#include <iomanip>

AnalysisResult VGAIsovistZone::run(Communicator *) {

    AnalysisResult result;

    auto &attributes = m_map.getAttributeTable();

    if (m_originPointSets.empty()) {
        return result;
    }
    int zoneColumnIndex = -1;
    for (auto originPointSet : m_originPointSets) {
        std::string originPointSetName = originPointSet.first;

        std::string zoneColumnName = getFormattedColumn( //
            Column::ISOVIST_ZONE_DISTANCE, originPointSetName, m_restrictDistance);
        std::string inverseZoneColumnName = getFormattedColumn( //
            Column::ISOVIST_ZONE_INV_SQ_DISTANCE, originPointSetName, m_restrictDistance);

        auto originPoints = originPointSet.second;

        zoneColumnIndex = attributes.insertOrResetColumn(zoneColumnName);
        result.addAttribute(zoneColumnName);

        for (PixelRef ref : originPoints) {
            AttributeRow &row = attributes.getRow(AttributeKey(ref));
            row.setValue(zoneColumnIndex, 0);
            Point &lp = m_map.getPoint(ref);
            std::set<MetricTriple> newPixels;
            extractMetric(lp.getNode(), newPixels, m_map, MetricTriple(0.0f, ref, NoPixel));
            for (auto &zonePixel : newPixels) {
                auto *zonePixelRow = attributes.getRowPtr(AttributeKey(zonePixel.m_pixel));
                if (zonePixelRow != 0) {
                    double zoneLineDist = dist(ref, zonePixel.m_pixel) * m_map.getSpacing();
                    float currZonePixelVal = zonePixelRow->getValue(zoneColumnIndex);
                    if ((currZonePixelVal == -1 || zoneLineDist < currZonePixelVal) &&
                        (m_restrictDistance <= 0 ||
                         (m_restrictDistance > 0 && zoneLineDist < m_restrictDistance))) {
                        zonePixelRow->setValue(zoneColumnIndex, zoneLineDist);
                    }
                }
            }
        }
        int inverseZoneColumnIndex = attributes.insertOrResetColumn(inverseZoneColumnName);
        setColumnFormulaAndUpdate(m_map, inverseZoneColumnIndex,
                                  "1/((value(\"" + zoneColumnName + "\") + 1) ^ 2)", std::nullopt);
        result.addAttribute(inverseZoneColumnName);
    }

    result.completed = true;

    return result;
}

void VGAIsovistZone::extractMetric(Node n, std::set<MetricTriple> &pixels, PointMap &map,
                                   const MetricTriple &curs) {
    for (int i = 0; i < 32; i++) {
        Bin &bin = n.bin(i);
        for (auto pixVec : bin.pixelVecs) {
            for (PixelRef pix = pixVec.start(); pix.col(bin.dir) <= pixVec.end().col(bin.dir);) {
                Point &pt = map.getPoint(pix);
                if (pt.filled()) {
                    pixels.insert(MetricTriple(0, pix, curs.m_pixel));
                }
                pix.move(bin.dir);
            }
        }
    }
}

void VGAIsovistZone::setColumnFormulaAndUpdate(PointMap &pointmap, int columnIndex,
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
            programCompleted = proggy.runupdate(columnIndex, selectionSet.value());
        } else {
            programCompleted = proggy.runupdate(columnIndex);
        }
        if (!programCompleted) {
            throw depthmapX::RuntimeException("There was an error parsing your formula:\n\n" +
                                              proggy.getLastErrorMessage());
        }
    }
    programContext.getTable()->getColumn(columnIndex).setFormula(formula);
}
