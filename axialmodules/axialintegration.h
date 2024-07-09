// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/stringutils.h"
#include "salalib/iaxial.h"

class AxialIntegration : IAxial {
  private:
    std::set<double> m_radius_set;
    int m_weighted_measure_col;
    bool m_choice;
    bool m_fulloutput;

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
    getFormattedColumn(std::string column, double radius,
                       std::optional<std::string> weightingColName = std::nullopt,
                       std::optional<std::string> normalisation = std::nullopt) {
        std::string colName = column;
        bool spaceAdded = false;
        if (weightingColName.has_value()) {
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
    static size_t getFormattedColumnIdx(AttributeTable &attributes, std::string column,
                                        double radius,
                                        std::optional<std::string> weightingColName = std::nullopt,
                                        std::optional<std::string> normalisation = std::nullopt) {
        return attributes.getColumnIndex(
            getFormattedColumn(column, radius, weightingColName, normalisation));
    }

  private:
    std::vector<int> getFormattedRadii(std::set<double> radiusSet);
    std::vector<std::string> getRequiredColumns(std::vector<int> radii,
                                                std::string weightingColName, bool simple_version);

  public:
    std::string getAnalysisName() const override { return "Angular Analysis"; }

    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
    AxialIntegration(std::set<double> radius_set, int weighted_measure_col, bool choice,
                     bool fulloutput)
        : m_radius_set(radius_set), m_weighted_measure_col(weighted_measure_col), m_choice(choice),
          m_fulloutput(fulloutput) {}
};
