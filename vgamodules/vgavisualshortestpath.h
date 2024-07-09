// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pixelref.h"
#include "salalib/pointmap.h"

#include "genlib/simplematrix.h"

class VGAVisualShortestPath : public IAnalysis {
  private:
    PointMap &m_map;
    PixelRef m_pixelFrom, m_pixelTo;
    void extractUnseen(Node &node, PixelRefVector &pixels, depthmapX::RowMatrix<int> &miscs,
                       depthmapX::RowMatrix<PixelRef> &extents);

  public:
    struct Column {
        inline static const std::string                                  //
            VISUAL_SHORTEST_PATH = "Visual Shortest Path",               //
            VISUAL_SHORTEST_PATH_LINKED = "Visual Shortest Path Linked", //
            VISUAL_SHORTEST_PATH_ORDER = "Visual Shortest Path Order",   //
            VISUAL_SHORTEST_PATH_ZONE = "Visual Shortest Path Zone";     //
    };

  public:
    std::string getAnalysisName() const override { return "Visibility Shortest Path"; }
    AnalysisResult run(Communicator *) override;
    VGAVisualShortestPath(PointMap &map, PixelRef pixelFrom, PixelRef pixelTo)
        : m_map(map), m_pixelFrom(pixelFrom), m_pixelTo(pixelTo) {}
};
