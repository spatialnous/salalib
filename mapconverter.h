// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "shapegraph.h"
#include "shapemap.h"
#include "spacepixfile.h"

#include "genlib/comm.h"

namespace MapConverter {

    std::unique_ptr<ShapeGraph>
    convertDrawingToAxial(Communicator *comm, const std::string &name,
                          const std::vector<SpacePixelFile> &drawingFiles);
    std::unique_ptr<ShapeGraph> convertDataToAxial(Communicator *comm, const std::string &name,
                                                   ShapeMap &shapemap, bool copydata = false);
    std::unique_ptr<ShapeGraph>
    convertDrawingToConvex(Communicator *, const std::string &name,
                           const std::vector<SpacePixelFile> &drawingFiles);
    std::unique_ptr<ShapeGraph> convertDataToConvex(Communicator *, const std::string &name,
                                                    ShapeMap &shapemap, bool copydata = false);
    std::unique_ptr<ShapeGraph>
    convertDrawingToSegment(Communicator *comm, const std::string &name,
                            const std::vector<SpacePixelFile> &drawingFiles);
    std::unique_ptr<ShapeGraph> convertDataToSegment(Communicator *comm, const std::string &name,
                                                     ShapeMap &shapemap, bool copydata = false);
    std::unique_ptr<ShapeGraph> convertAxialToSegment(Communicator *, ShapeGraph &axialMap,
                                                      const std::string &name,
                                                      bool keeporiginal = true,
                                                      bool pushvalues = false,
                                                      double stubremoval = 0.0);

} // namespace MapConverter
