// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ivga.h"
#include "salalib/pointdata.h"

#include "genlib/bsptree.h"

class VGAIsovist : IVGA {
    const std::vector<SalaShape> &m_boundaryShapes;

  public:
    VGAIsovist(const std::vector<SalaShape> &boundaryShapes) : m_boundaryShapes(boundaryShapes) {}
    std::string getAnalysisName() const override { return "Isovist Analysis"; }
    AnalysisResult run(Communicator *comm, PointMap &map, bool simple_version) override;

  private:
    std::vector<std::pair<std::string, int>> createAttributes(AttributeTable &table,
                                                              bool simple_version);
    std::set<std::string> setData(Isovist &isovist, AttributeRow &row,
                                  std::vector<std::pair<std::string, int>> cols,
                                  bool simple_version);
    BSPNode makeBSPtree(Communicator *communicator, const std::vector<SalaShape> &boundaryShapes);
};
