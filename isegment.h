// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of VGA analysis

#include "analysisresult.h"
#include "radiustype.h"
#include "shapegraph.h"

#include "genlib/comm.h"
#include "genlib/stringutils.h"

#include <string>

class ISegment {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual AnalysisResult run(Communicator *comm, ShapeGraph &map, bool simpleVersion) = 0;
    virtual ~ISegment() {}

    // Axial map helper: convert a radius for angular analysis

    static std::string makeFloatRadiusText(double radius) {
        std::string radiusText;
        if (radius > 100.0) {
            radiusText = dXstring::formatString(radius, "%.f");
        } else if (radius < 0.1) {
            radiusText = dXstring::formatString(radius, "%.4f");
        } else {
            radiusText = dXstring::formatString(radius, "%.2f");
        }
        return radiusText;
    }

    static std::string makeRadiusText(RadiusType radiusType, double radius) {
        std::string radiusText;
        if (radius != -1) {
            if (radiusType == RadiusType::TOPOLOGICAL) {
                radiusText =
                    std::string(" R") + dXstring::formatString(int(radius), "%d") + " step";
            } else if (radiusType == RadiusType::METRIC) {
                radiusText = std::string(" R") + makeFloatRadiusText(radius) + " metric";
            } else { // radius angular
                radiusText = std::string(" R") + makeFloatRadiusText(radius);
            }
        }
        return radiusText;
    }
};
