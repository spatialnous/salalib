// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/ianalysis.h"
#include "salalib/pointdata.h"

class ExtractLinkData : public IAnalysis {
  private:
    PointMap &m_map;

  public:
    std::string getAnalysisName() const override { return "Extract Link Data"; }
    ExtractLinkData(PointMap &map) : m_map(map) {}
    AnalysisResult run(Communicator *comm) override;
};
