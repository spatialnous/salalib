// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/isegment.h"

class SegmentAngular : ISegment {
  private:
    std::set<double> m_radiusSet;

  public:
    struct Column {
        inline static const std::string                  //
            ANGULAR_MEAN_DEPTH = "Angular Mean Depth",   //
            ANGULAR_NODE_COUNT = "Angular Node Count",   //
            ANGULAR_TOTAL_DEPTH = "Angular Total Depth"; //
    };
    static std::string getFormattedColumn(std::string column, double radius) {
        std::string colName = column;
        if (radius != -1.0) {
            colName += makeRadiusText(RadiusType::ANGULAR, radius);
        }
        return colName;
    }

  public:
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    AnalysisResult run(Communicator *comm, ShapeGraph &map, bool) override;
    SegmentAngular(std::set<double> radiusSet) : m_radiusSet(radiusSet) {}
};
