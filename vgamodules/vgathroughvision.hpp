// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivga.hpp"

#include "../pointmap.hpp"

class VGAThroughVision : public IVGA {
  protected:
    struct AnalysisData {
        const Point &point;
        const PixelRef ref;
        int misc = 0;
        size_t attributeDataRow;
        AnalysisData(const Point &pointIn, const PixelRef refIn, size_t attributeDataRowIn,
                     int miscIn = 0)
            : point(pointIn), ref(refIn), misc(miscIn), attributeDataRow(attributeDataRowIn) {}
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
