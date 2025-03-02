// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../genlib/stringutils.hpp"
#include "../iaxial.hpp"

class AxialIntegration : IAxial {
    std::set<double> m_radiusSet;
    std::optional<size_t> m_weightedMeasureCol;
    bool m_choice;
    bool m_fulloutput;
    bool m_forceLegacyColumnOrder = false;

    [[maybe_unused]] unsigned _padding0 : 1 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    struct Normalisation {
        inline static const std::string //
            NORM = "Norm",              //
            HH = "HH",                  //
            PV = "P-value",             //
            TK = "Tekl",                //
            PENN = "Penn";              //
    };
    struct Column {
        inline static const std::string                  //
            CHOICE = "Choice",                           //
            ENTROPY = "Entropy",                         //
            METRIC_NODE_COUNT = "Metric Node Count",     //
            INTEGRATION = "Integration",                 //
            INTENSITY = "Intensity",                     //
            HARMONIC_MEAN_DEPTH = "Harmonic Mean Depth", //
            MEAN_DEPTH = "Mean Depth",                   //
            NODE_COUNT = "Node Count",                   //
            RELATIVISED_ENTROPY = "Relativised Entropy", //
            TOTAL = "Total",                             //
            RA = "RA",                                   //
            RRA = "RRA",                                 //
            TOTAL_DEPTH = "Total Depth";                 //
    };
    static std::string
    getFormattedColumn(const std::string &column, int radius,
                       const std::optional<std::string> &weightingColName = std::nullopt,
                       const std::optional<std::string> &normalisation = std::nullopt) {
        std::string colName = column;
        bool spaceAdded = false;
        if (weightingColName.has_value() && column == Column::TOTAL) {
            // The TOTAL column seems to be special i.e. not really a weighting
            colName += " " + weightingColName.value();
            spaceAdded = true;
        } else if (weightingColName.has_value()) {
            colName += " [" + weightingColName.value() + " Wgt]";
            spaceAdded = true;
        }
        if (normalisation.has_value()) {
            if (!spaceAdded) {
                colName += " ";
            }
            colName += "[" + normalisation.value() + "]";
        }
        if (radius != -1.0) {
            colName += dXstring::formatString(radius, " R%d");
        }
        return colName;
    }
    static size_t
    getFormattedColumnIdx(const AttributeTable &attributes, const std::string &column, int radius,
                          const std::optional<std::string> &weightingColName = std::nullopt,
                          const std::optional<std::string> &normalisation = std::nullopt) {
        return attributes.getColumnIndex(
            getFormattedColumn(column, radius, weightingColName, normalisation));
    }

  private:
    static std::vector<int> getFormattedRadii(std::set<double> radiusSet);
    std::vector<std::string> getRequiredColumns(std::vector<int> radii,
                                                std::string weightingColName, bool simpleVersion);

  public:
    AxialIntegration(std::set<double> radiusSet, int weightedMeasureCol, bool choice,
                     bool fulloutput)
        : m_radiusSet(std::move(radiusSet)),
          m_weightedMeasureCol(weightedMeasureCol < 0 ? std::nullopt
                                                      : std::make_optional(weightedMeasureCol)),
          m_choice(choice), m_fulloutput(fulloutput), _padding0(0), _padding1(0) {}
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    void setForceLegacyColumnOrder(bool forceLegacyColumnOrder) {
        m_forceLegacyColumnOrder = forceLegacyColumnOrder;
    }
    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
};
