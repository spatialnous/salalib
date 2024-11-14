// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgavisual.h"

#include "../pointmap.h"

#include "../genlib/stringutils.h"

class VGAVisualGlobal : public IVGAVisual {
  private:
    double m_radius;
    bool m_gatesOnly;
    bool m_simpleVersion = false;

    // To maintain binary compatibility with older .graph versions
    // write the last "misc" values back to the points
    bool m_legacyWriteMiscs = false;

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

  private:
    std::vector<std::string> getColumns(bool simpleVersion) const;

  public:
    VGAVisualGlobal(const PointMap &map, double radius, bool gatesOnly)
        : IVGAVisual(map), m_radius(radius), m_gatesOnly(gatesOnly) {}
    std::string getAnalysisName() const override { return "Global Visibility Analysis"; }
    AnalysisResult run(Communicator *comm) override;

  public:
    void setSimpleVersion(bool simpleVersion) { m_simpleVersion = simpleVersion; }
    void setLegacyWriteMiscs(bool legacyWriteMiscs) { m_legacyWriteMiscs = legacyWriteMiscs; }
};
