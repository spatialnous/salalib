// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgaangular.h"
#include "salalib/pointmap.h"

#include "genlib/stringutils.h"

class VGAAngular : public IVGAAngular {
  private:
    double m_radius;
    bool m_gatesOnly;

  public:
    struct Column {
        inline static const std::string                  //
            ANGULAR_MEAN_DEPTH = "Angular Mean Depth",   //
            ANGULAR_TOTAL_DEPTH = "Angular Total Depth", //
            ANGULAR_NODE_COUNT = "Angular Node Count";   //
    };
    static std::string getColumnWithRadius(std::string column, double radius, QtRegion mapRegion) {
        if (radius != -1.0) {
            if (radius > 100.0) {
                return column + " R" + dXstring::formatString(radius, "%.f");
            } else if (mapRegion.width() < 1.0) {
                return column + " R" + dXstring::formatString(radius, "%.4f");
            } else {
                return column + " R" + dXstring::formatString(radius, "%.2f");
            }
        }
        return column;
    }

  public:
    VGAAngular(const PointMap &map, double radius, bool gatesOnly)
        : IVGAAngular(map), m_radius(radius), m_gatesOnly(gatesOnly) {}
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    AnalysisResult run(Communicator *comm) override;
};
