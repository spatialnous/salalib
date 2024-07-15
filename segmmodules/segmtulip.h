// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentTulip : ISegment {
  private:
    std::set<double> m_radius_set;
    std::optional<const std::set<int>> m_selSet;
    int m_tulip_bins;
    int m_weighted_measure_col;
    int m_weighted_measure_col2;
    int m_routeweight_col;
    RadiusType m_radius_type;
    bool m_choice;
    bool m_interactive;
    bool m_forceLegacyColumnOrder = false;

  public:
    struct Column {
        inline static const std::string  //
            CHOICE = "Choice",           //
            INTEGRATION = "Integration", //
            NODE_COUNT = "Node Count",   //
            TOTAL_DEPTH = "Total Depth", //
            TOTAL = "Total";             //
    };
    static std::string
    getFormattedColumn(std::string column, int tulip_bins, RadiusType radiusType, double radius,
                       std::optional<std::string> routeWeightColName = std::nullopt,
                       std::optional<std::string> weightCol1Name = std::nullopt,
                       std::optional<std::string> weightCol2Name = std::nullopt) {
        std::string colName = "T" + dXstring::formatString(tulip_bins, "%d") + " " + column;
        bool spaceAdded = false;
        if (routeWeightColName.has_value() && weightCol1Name.has_value()) {
            colName += " [";
            spaceAdded = true;
        }
        if (routeWeightColName.has_value()) {
            if (!spaceAdded)
                colName += " ";
            colName += "[Route weight by " + routeWeightColName.value() + " Wgt]";
            spaceAdded = true;
        }
        if (weightCol1Name.has_value() && column == Column::TOTAL) {
            if (!spaceAdded)
                colName += " ";
            colName += weightCol1Name.value();
        } else if (weightCol1Name.has_value() && !weightCol2Name.has_value()) {
            if (!spaceAdded)
                colName += " ";
            colName += "[" + weightCol1Name.value() + " Wgt]";
        } else if (weightCol1Name.has_value() && weightCol2Name.has_value()) {
            if (!spaceAdded)
                colName += " ";
            // TODO: Should there be a space between the two metrics?
            colName += " [" + weightCol1Name.value() + "-" + weightCol2Name.value() + " Wgt]";
        }

        if (routeWeightColName.has_value() && weightCol1Name.has_value()) {
            colName += "]";
        }
        if (radius != -1.0) {
            // TODO: This should end in "topological" not "metric"
            colName += makeRadiusText(radiusType, radius);
        }
        return colName;
    }
    static size_t
    getFormattedColumnIdx(AttributeTable &attributes, std::string column, int tulip_bins,
                          RadiusType radiusType, double radius,
                          std::optional<std::string> weightCol1Name = std::nullopt,
                          std::optional<std::string> weightCol2Name = std::nullopt,
                          std::optional<std::string> routeWeightColName = std::nullopt) {
        return attributes.getColumnIndex(getFormattedColumn(column, tulip_bins, radiusType, radius,
                                                            weightCol1Name, weightCol2Name,
                                                            routeWeightColName));
    }

  private:
    std::vector<std::string> getRequiredColumns(ShapeGraph &map, std::vector<double> radii);

  public:
    SegmentTulip(std::set<double> radius_set, std::optional<const std::set<int>> selSet,
                 int tulip_bins, int weighted_measure_col, RadiusType radius_type, bool choice,
                 bool interactive = false, int weighted_measure_col2 = -1, int routeweight_col = -1)
        : m_radius_set(radius_set), m_selSet(selSet), m_tulip_bins(tulip_bins),
          m_weighted_measure_col(weighted_measure_col),
          m_weighted_measure_col2(weighted_measure_col2), m_routeweight_col(routeweight_col),
          m_radius_type(radius_type), m_choice(choice), m_interactive(interactive) {}
    void setForceLegacyColumnOrder(bool forceLegacyColumnOrder) {
        m_forceLegacyColumnOrder = forceLegacyColumnOrder;
    }
    std::string getAnalysisName() const override { return "Tulip Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
};
