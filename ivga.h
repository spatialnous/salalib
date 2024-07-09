// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of VGA analysis

#include "analysisresult.h"
#include "pointmap.h"

#include "genlib/comm.h"

#include <string>

class IVGA {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual AnalysisResult run(Communicator *comm, PointMap &map, bool simple_version) = 0;
    virtual ~IVGA() {}
};
