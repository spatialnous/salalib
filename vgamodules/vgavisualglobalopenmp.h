// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "genlib/stringutils.h"
#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"

#include "genlib/simplematrix.h"

class VGAVisualGlobalOpenMP : public IAnalysis {
  private:
    PointMap &m_map;
    double m_radius;
    bool m_gatesOnly;

    struct DataPoint {
        float count, depth, integ_dv, integ_pv;
        float integ_tk, entropy, rel_entropy;
    };
    void extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                       depthmapX::RowMatrix<PixelRef> &extents);

  public:
    struct Column {
        inline static const std::string                             //
            VISUAL_ENTROPY = "Visual Entropy",                      //
            VISUAL_INTEGRATION_HH = "Visual Integration [HH]",      //
            VISUAL_INTEGRATION_PV = "Visual Integration [P-value]", //
            VISUAL_INTEGRATION_TK = "Visual Integration [Tekl]",    //
            VISUAL_MEAN_DEPTH = "Visual Mean Depth",                //
            VISUAL_NODE_COUNT = "Visual Node Count",                //
            VISUAL_REL_ENTROPY = "Visual Relativised Entropy";      //
    };
    static std::string getColumnWithRadius(std::string column, double radius) {
        if (radius != -1) {
            return column + " R" + dXstring::formatString(int(radius), "%d");
        }
        return column;
    }

  public:
    VGAVisualGlobalOpenMP(PointMap &map, double radius, bool gatesOnly)
        : m_map(map), m_radius(radius), m_gatesOnly(gatesOnly) {}
    std::string getAnalysisName() const override { return "Global Visibility Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *) override;
};
