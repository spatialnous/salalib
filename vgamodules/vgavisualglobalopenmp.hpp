// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgavisual.hpp"

#include "../genlib/stringutils.hpp"
#include "../pointmap.hpp"

class VGAVisualGlobalOpenMP : public IVGAVisual {
    double m_radius;
    std::optional<int> m_limitToThreads;
    bool m_gatesOnly;
    bool m_forceCommUpdatesMasterThread = false;

    // To maintain binary compatibility with older .graph versions
    // write the last "misc" values back to the points
    bool m_legacyWriteMiscs = false;

    [[maybe_unused]] unsigned _padding0 : 1 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

    struct DataPoint {
        float count, depth, integDv, integPv;
        float integTk, entropy, relEntropy;
    };

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
            return column + " R" + dXstring::formatString(static_cast<int>(radius), "%d");
        }
        return column;
    }

  public:
    VGAVisualGlobalOpenMP(PointMap &map, double radius, bool gatesOnly,
                          std::optional<int> limitToThreads = std::nullopt,
                          bool forceCommUpdatesMasterThread = false)
        : IVGAVisual(map), m_radius(radius), m_limitToThreads(limitToThreads),
          m_gatesOnly(gatesOnly), m_forceCommUpdatesMasterThread(forceCommUpdatesMasterThread),
          _padding0(0), _padding1(0) {}
    std::string getAnalysisName() const override { return "Global Visibility Analysis (OpenMP)"; }
    AnalysisResult run(Communicator *) override;

  public:
    void setLegacyWriteMiscs(bool legacyWriteMiscs) { m_legacyWriteMiscs = legacyWriteMiscs; }
};
