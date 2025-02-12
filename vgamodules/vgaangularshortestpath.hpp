// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ivgaangular.hpp"

#include "../pixelref.hpp"
#include "../pointmap.hpp"

class VGAAngularShortestPath : public IVGAAngular {
  private:
    PixelRef m_pixelFrom, m_pixelTo;

  public:
    struct Column {
        inline static const std::string                                                      //
            ANGULAR_SHORTEST_PATH = "Angular Shortest Path",                                 //
            ANGULAR_SHORTEST_PATH_LINKED = "Angular Shortest Path Linked",                   //
            ANGULAR_SHORTEST_PATH_ORDER = "Angular Shortest Path Order",                     //
            ANGULAR_SHORTEST_PATH_VISUAL_ZONE = "Angular Shortest Path Visual Zone",         //
            ANGULAR_SHORTEST_PATH_METRIC_ZONE = "Angular Shortest Path Metric Zone",         //
            ANGULAR_SHORTEST_PATH_INV_METRIC_ZONE = "Angular Shortest Path Inv Metric Zone"; //
    };

  public:
    std::string getAnalysisName() const override { return "Angular Shortest Path"; }
    AnalysisResult run(Communicator *comm) override;
    VGAAngularShortestPath(PointMap &map, PixelRef pixelFrom, PixelRef pixelTo)
        : IVGAAngular(map), m_pixelFrom(pixelFrom), m_pixelTo(pixelTo) {}
};
