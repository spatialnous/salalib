// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of VGA analysis

#include "analysisresult.h"
#include "options.h"
#include "shapegraph.h"

#include "genlib/comm.h"
#include "genlib/stringutils.h"

#include <string>

class ISegment {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual AnalysisResult run(Communicator *comm, ShapeGraph &map, bool simple_version) = 0;
    virtual ~ISegment() {}

    // Axial map helper: convert a radius for angular analysis

    static std::string makeFloatRadiusText(double radius) {
        std::string radius_text;
        if (radius > 100.0) {
            radius_text = dXstring::formatString(radius, "%.f");
        } else if (radius < 0.1) {
            radius_text = dXstring::formatString(radius, "%.4f");
        } else {
            radius_text = dXstring::formatString(radius, "%.2f");
        }
        return radius_text;
    }

    static std::string makeRadiusText(int radius_type, double radius) {
        std::string radius_text;
        if (radius != -1) {
            if (radius_type == Options::RADIUS_STEPS) {
                radius_text =
                    std::string(" R") + dXstring::formatString(int(radius), "%d") + " step";
            } else if (radius_type == Options::RADIUS_METRIC) {
                radius_text = std::string(" R") + makeFloatRadiusText(radius) + " metric";
            } else { // radius angular
                radius_text = std::string(" R") + makeFloatRadiusText(radius);
            }
        }
        return radius_text;
    }
};
