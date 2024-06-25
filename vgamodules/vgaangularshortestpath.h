// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointdata.h"

class VGAAngularShortestPath : public IAnalysis {
  private:
    PointMap &m_map;
    PixelRef m_pixelFrom, m_pixelTo;

  public:
    std::string getAnalysisName() const override { return "Angular Shortest Path"; }
    AnalysisResult run(Communicator *comm) override;
    VGAAngularShortestPath(PointMap &map, PixelRef pixelFrom, PixelRef pixelTo)
        : m_map(map), m_pixelFrom(pixelFrom), m_pixelTo(pixelTo) {}
};
