// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgavisual.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"

class VGAVisualShortestPath : public IVGAVisual {
  private:
    PixelRef m_pixelFrom, m_pixelTo;

  public:
    struct Column {
        inline static const std::string                                                    //
            VISUAL_SHORTEST_PATH = "Visual Shortest Path",                                 //
            VISUAL_SHORTEST_PATH_LINKED = "Visual Shortest Path Linked",                   //
            VISUAL_SHORTEST_PATH_ORDER = "Visual Shortest Path Order",                     //
            VISUAL_SHORTEST_PATH_VISUAL_ZONE = "Visual Shortest Path Visual Zone",         //
            VISUAL_SHORTEST_PATH_METRIC_ZONE = "Visual Shortest Path Metric Zone",         //
            VISUAL_SHORTEST_PATH_INV_METRIC_ZONE = "Visual Shortest Path Inv Metric Zone"; //
    };

  public:
    std::string getAnalysisName() const override { return "Visibility Shortest Path"; }
    AnalysisResult run(Communicator *) override;
    VGAVisualShortestPath(const PointMap &map, PixelRef pixelFrom, PixelRef pixelTo)
        : IVGAVisual(map), m_pixelFrom(pixelFrom), m_pixelTo(pixelTo) {}
};
