// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../isegment.hpp"

// This is the legacy Tulip angular analysis where Choice is only calculated
// from leaf nodes. These are not the traditional "dead end" nodes of graph
// analysis, rather, they are ends of traversal that may happen even if
// (during traversal) we encounter already traversed nodes.

class SegmentTulipLeafChoice : ISegment {
  private:
    std::set<double> m_radiusSet;
    std::optional<std::set<int>> m_selSet;
    int m_tulipBins;
    int m_weightedMeasureCol;
    int m_weightedMeasureCol2;
    int m_routeweightCol;
    RadiusType m_radiusType;
    bool m_recordSelLeafs;
    bool m_interactive;
    bool m_forceLegacyColumnOrder = false;

    [[maybe_unused]] unsigned _padding0 : 1 * 8;

    // used during angular analysis
    struct AnalysisInfo {
        // lists used for multiple radius analysis
        bool leaf;
        bool choicecovered;

      private:
        [[maybe_unused]] unsigned _padding0 : 2 * 8;

      public:
        SegmentRef previous;
        int depth;
        double choice;
        double weightedChoice;
        double weightedChoice2; // EFEF
        AnalysisInfo()
            : leaf(true), choicecovered(false), _padding0(0), previous(), depth(0), choice(0.0),
              weightedChoice(0.0), weightedChoice2(0.0) {

            previous = SegmentRef();
        }
        void clearLine() {
            choicecovered = false;
            leaf = true;
            previous = SegmentRef();
            depth = 0; // choice values are cummulative and not cleared
        }
    };

  public:
    struct Column {
        inline static const std::string  //
            LEAF_CHOICE = "Leaf Choice", //
            LEAF = "Leaf";               //
    };
    static std::string
    getFormattedColumn(const std::string &column, int tulipBins, RadiusType radiusType,
                       double radius, bool selectionOnly,
                       const std::optional<std::string> &routeWeightColName = std::nullopt,
                       const std::optional<std::string> &weightCol1Name = std::nullopt,
                       const std::optional<std::string> &weightCol2Name = std::nullopt,
                       const std::optional<int> &leafRef = std::nullopt) {
        std::string colName = "T" + dXstring::formatString(tulipBins, "%d") + " " + column;
        if (leafRef.has_value()) {
            colName += "_" + std::to_string(*leafRef);
        }
        if (selectionOnly) {
            colName += " (Selection)";
        }
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
        if (weightCol1Name.has_value() && !weightCol2Name.has_value()) {
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
    getFormattedColumnIdx(const AttributeTable &attributes, std::string column, int tulipBins,
                          RadiusType radiusType, double radius, bool selectionOnly,
                          const std::optional<std::string> &weightCol1Name = std::nullopt,
                          const std::optional<std::string> &weightCol2Name = std::nullopt,
                          const std::optional<std::string> &routeWeightColName = std::nullopt,
                          const std::optional<int> &leafRef = std::nullopt) {
        return attributes.getColumnIndex(
            getFormattedColumn(column, tulipBins, radiusType, radius, selectionOnly, weightCol1Name,
                               weightCol2Name, routeWeightColName, leafRef));
    }

  private:
    std::vector<std::string> getRequiredColumns(ShapeGraph &map, std::vector<double> radii);

  public:
    SegmentTulipLeafChoice(std::set<double> radiusSet, std::optional<std::set<int>> selSet,
                           int tulipBins, int weightedMeasureCol, RadiusType radiusType,
                           int weightedMeasureCol2 = -1, int routeweightCol = -1,
                           bool recordSelLeafs = false, bool interactive = false)
        : m_radiusSet(std::move(radiusSet)), m_selSet(std::move(selSet)), m_tulipBins(tulipBins),
          m_weightedMeasureCol(weightedMeasureCol), m_weightedMeasureCol2(weightedMeasureCol2),
          m_routeweightCol(routeweightCol), m_radiusType(radiusType),
          m_recordSelLeafs(recordSelLeafs), m_interactive(interactive), _padding0(0) {}
    void setForceLegacyColumnOrder(bool forceLegacyColumnOrder) {
        m_forceLegacyColumnOrder = forceLegacyColumnOrder;
    }
    std::string getAnalysisName() const override { return "Tulip Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
};
