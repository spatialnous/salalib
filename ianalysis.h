// SPDX-FileCopyrightText: 2020-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "salalib/analysisresult.h"

#include "genlib/comm.h"

#include <set>
#include <string>

class IAnalysis {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual AnalysisResult run(Communicator *comm) = 0;
    virtual ~IAnalysis() {}
};
