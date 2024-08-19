// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of Axial analysis

#include "salalib/analysisresult.h"
#include "salalib/shapegraph.h"

#include "genlib/comm.h"

#include <string>

class IAxial {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual AnalysisResult run(Communicator *comm, ShapeGraph &map, bool simpleVersion) = 0;
    virtual ~IAxial() {}
};
