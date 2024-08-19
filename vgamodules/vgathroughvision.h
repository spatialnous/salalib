// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivga.h"

#include "salalib/pointmap.h"

class VGAThroughVision : public IVGA {
  protected:
    struct AnalysisData {
        const Point &point;
        const PixelRef ref;
        size_t attributeDataRow;
        int misc = 0;
        AnalysisData(const Point &point, const PixelRef ref, size_t attributeDataRow, int misc = 0)
            : point(point), ref(ref), attributeDataRow(attributeDataRow), misc(misc) {}
    };

  public:
    struct Column {
        inline static const std::string        //
            THROUGH_VISION = "Through vision"; //
    };

  public:
    VGAThroughVision(const PointMap &map) : IVGA(map) {}
    std::string getAnalysisName() const override { return "Through Vision Analysis"; }
    AnalysisResult run(Communicator *comm) override;
};
