// SPDX-FileCopyrightText: 2018-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface to handle different kinds of Axial analysis

#include "analysisresult.hpp"
#include "shapegraph.hpp"

#include "genlib/comm.hpp"

#include <string>

class IAxial {
  public:
    virtual std::string getAnalysisName() const = 0;
    virtual AnalysisResult run(Communicator *comm, ShapeGraph &map, bool simpleVersion) = 0;
    virtual ~IAxial() {}
};
