// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgaangular.hpp"

#include "../genlib/stringutils.hpp"
#include "../pointmap.hpp"

class VGAAngular : public IVGAAngular {
    double m_radius;
    bool m_gatesOnly;

    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    struct Column {
        inline static const std::string                  //
            ANGULAR_MEAN_DEPTH = "Angular Mean Depth",   //
            ANGULAR_TOTAL_DEPTH = "Angular Total Depth", //
            ANGULAR_NODE_COUNT = "Angular Node Count";   //
    };
    static std::string getColumnWithRadius(std::string column, double radius, Region4f mapRegion) {
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
        : IVGAAngular(map), m_radius(radius), m_gatesOnly(gatesOnly), _padding0(0), _padding1(0) {}
    std::string getAnalysisName() const override { return "Angular Analysis"; }
    AnalysisResult run(Communicator *comm) override;
};
