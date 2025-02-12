// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../iaxial.h"

class AxialLocal : IAxial {
  public:
    struct Column {
        inline static const std::string          //
            CONTROL = "Control",                 //
            CONTROLLABILITY = "Controllability"; //
    };

  public:
    std::string getAnalysisName() const override { return "Angular Local Analysis"; }

    AnalysisResult run(Communicator *, ShapeGraph &map, bool) override;
    AxialLocal() {}
};
