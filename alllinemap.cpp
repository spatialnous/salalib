// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "alllinemap.h"

#include "axialminimiser.h"
#include "tolerances.h"

#include "genlib/exceptions.h"

#include <iomanip>
#include <time.h>

AllLine::MapData
AllLine::generate(Communicator *comm, ShapeGraph &map,
                  const std::vector<std::reference_wrapper<const ShapeMap>> &drawingLayers,
                  const Point2f &seed) {
    MapData mapData;
    generate(comm, map, mapData, drawingLayers, seed);
    return mapData;
}

void AllLine::generate(Communicator *comm, ShapeGraph &map, MapData &mapData,
                       const std::vector<std::reference_wrapper<const ShapeMap>> &drawingLayers,
                       const Point2f &seed) {

    std::vector<Line> lines;
    QtRegion region;

    // add all visible layers to the set of polygon lines...
    for (auto map : drawingLayers) {
        if (region.atZero()) {
            region = map.get().getRegion();
        } else {
            region = runion(region, map.get().getRegion());
        }
        std::vector<SimpleLine> newLines = map.get().getAllShapesAsSimpleLines();
        for (const auto &line : newLines) {
            lines.push_back(Line(line.start(), line.end()));
        }
    }
    generate(comm, map, mapData, lines, region, seed);
}

AllLine::MapData AllLine::generate(Communicator *comm, ShapeGraph &map, std::vector<Line> &lines,
                                   QtRegion &region, const Point2f &seed) {
    MapData mapData;
    generate(comm, map, mapData, lines, region, seed);
    return mapData;
}

void AllLine::generate(Communicator *comm, ShapeGraph &map, AllLine::MapData &mapData,
                       std::vector<Line> &lines, QtRegion &region, const Point2f &seed) {
    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 3);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }
    // this has a nasty habit of crashing if reused...
    // reset everything at the top level, including any existing all-line map:
    mapData.polygons.clear();
    mapData.polyConnections.clear();
    mapData.radialLines.clear();

    // starting off... finding a polygon...
    // for ease, I'm just going to make a construction line set from all the
    // visible lines...

    region.grow(1.30);
    mapData.polygons.init(lines, region);
    mapData.polygons.m_handled_list.clear();

    // find a corner visible from the seed:
    AxialVertexKey seedvertex = mapData.polygons.seedVertex(seed);

    if (seedvertex == NoVertex) {
        // oops... can't find a visible vertex
        throw depthmapX::RuntimeException("No visible vertices found");
    }

    // okay, we've got as far as finding a seed corner, now the real fun begins...
    // test outwards from corner, add other corners to
    // test set...
    std::vector<Line> axiallines;
    KeyVertices preaxialdata;
    // also poly_connections used in fewest line axial map construction:
    mapData.polyConnections.clear();
    mapData.radialLines.clear();

    AxialVertex vertex = mapData.polygons.makeVertex(seedvertex, seed);
    if (!vertex.m_initialised) {
        // oops... can't init for some reason
        throw depthmapX::RuntimeException("Failed to initialise axial vertices");
    }

    time_t atime = 0;
    int count = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);
        comm->CommPostMessage(Communicator::NUM_RECORDS,
                              mapData.polygons.m_vertex_possibles.size());
    }

    std::set<AxialVertex> openvertices;
    openvertices.insert(vertex);
    while (!openvertices.empty()) {
        mapData.polygons.makeAxialLines(openvertices, axiallines, preaxialdata,
                                        mapData.polyConnections, mapData.radialLines);
        count++;
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, count);
            }
        }
    }

    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_STEP, 3);
        comm->CommPostMessage(Communicator::CURRENT_RECORD, 0);
    }

    // cut out duplicates:
    for (size_t j = 0; j < axiallines.size(); j++) {
        for (size_t k = axiallines.size() - 1; k > j; k--) {
            double maxdim = __max(region.width(), region.height());
            if (approxeq(axiallines[j].start(), axiallines[k].start(), maxdim * TOLERANCE_B) &&
                approxeq(axiallines[j].end(), axiallines[k].end(), maxdim * TOLERANCE_B)) {
                for (int preaxiali : preaxialdata[k]) {
                    preaxialdata[j].insert(preaxiali);
                }
                preaxialdata.erase(preaxialdata.begin() + int(k));
                axiallines.erase(axiallines.begin() + int(k));
            }
        }
    }

    region.grow(0.99); // <- this paired with crop code below to prevent error
    map.init(axiallines.size(),
             mapData.polygons.getRegion()); // used to be double density here
    map.initialiseAttributesAxial();
    for (size_t k = 0; k < axiallines.size(); k++) {
        axiallines[k].crop(region); // <- should be cropped anyway, but causing an error
        map.makeLineShape(axiallines[k]);
    }

    // n.b. make connections also initialises attributes
    // -> don't know what this was for: alllinemap.sortBins(m_poly_connections);
    map.makeConnections(preaxialdata);

    map.setKeyVertexCount(mapData.polygons.m_vertex_possibles.size());
}

std::tuple<ShapeGraph, ShapeGraph>
AllLine::extractFewestLineMaps(Communicator *comm, ShapeGraph &map, MapData &mapData) {

    if (comm) {
        comm->CommPostMessage(Communicator::NUM_STEPS, 2);
        comm->CommPostMessage(Communicator::CURRENT_STEP, 1);
    }

    pafmath::pafsrand((unsigned int)time(NULL));

    // make one rld for each radial line...
    std::map<RadialKey, std::set<int>> radialdivisions;
    size_t i;
    for (auto &radial_line : mapData.radialLines) {
        radialdivisions.insert(std::make_pair((RadialKey)radial_line, std::set<int>()));
    }

    // also, a list of radial lines cut by each axial line
    std::map<int, std::set<int>> ax_radial_cuts;
    std::map<int, std::set<int>> ax_seg_cuts;
    for (const auto &shape : map.getAllShapes()) {
        ax_radial_cuts.insert(std::make_pair(shape.first, std::set<int>()));
        ax_seg_cuts.insert(std::make_pair(shape.first, std::set<int>()));
    }

    // make divisions -- this is the slow part and the comm updates
    makeDivisions(map, mapData.polyConnections, mapData.radialLines, radialdivisions,
                  ax_radial_cuts, comm);

    // the slow part is over, we're into the final straight... reset the current
    // record flag:
    if (comm) {
        comm->CommPostMessage(Communicator::CURRENT_STEP, 2);
        comm->CommPostMessage(Communicator::CURRENT_RECORD, 0);
    }

    // a little further setting up is still required...
    std::map<RadialKey, RadialSegment> radialsegs;

    auto iter = mapData.radialLines.begin();
    if (iter != mapData.radialLines.end()) {
        // now make radial segments from the radial lines... (note, start at 1)
        auto prevIter = mapData.radialLines.begin();
        ++iter;
        for (; iter != mapData.radialLines.end();) {
            if (iter->vertex == prevIter->vertex && iter->ang != prevIter->ang) {
                radialsegs.insert(std::make_pair((RadialKey)(*iter), (RadialSegment)(*prevIter)));
            }
            ++iter;
            ++prevIter;
        }
    }

    // and segment divisors from the axial lines...
    // TODO: (CS) Restructure this to get rid of all those brittle parallel data
    // structure
    auto axIter = ax_radial_cuts.begin();
    auto axSeg = ax_seg_cuts.begin();
    for (i = 0; i < map.getAllShapes().size(); i++) {
        auto axRadCutIter = axIter->second.begin();
        if (axRadCutIter != axIter->second.end()) {
            auto axRadCutIterPrev = axIter->second.begin();
            ++axRadCutIter;
            for (size_t j = 1; j < axIter->second.size(); ++j) {
                // note similarity to loop above
                RadialKey &rk_end = mapData.radialLines[size_t(*axRadCutIter)];
                RadialKey &rk_start = mapData.radialLines[size_t(*axRadCutIterPrev)];
                if (rk_start.vertex == rk_end.vertex) {
                    auto radialSegIter = radialsegs.find(rk_end);
                    if (radialSegIter != radialsegs.end() &&
                        rk_start == radialSegIter->second.radial_b) {
                        radialSegIter->second.indices.insert(axIter->first);
                        axSeg->second.insert(std::distance(radialsegs.begin(), radialSegIter));
                    }
                }
                ++axRadCutIter;
                ++axRadCutIterPrev;
            }
        }
        axIter++;
        axSeg++;
    }

    // and a little more setting up: key vertex relationships
    std::vector<std::vector<int>> keyvertexconns;
    std::vector<int> keyvertexcounts(static_cast<size_t>(map.getKeyVertexCount()), 0);
    // this sets up a two step relationship: looks for the key vertices for all
    // lines connected to you
    auto connectors = map.getConnections();
    for (size_t y = 0; y < connectors.size(); y++) {
        keyvertexconns.push_back(std::vector<int>());
        auto &conn = keyvertexconns.back();
        Connector &axa = connectors[y];
        for (size_t z = 0; z < axa.m_connections.size(); z++) {
            std::set<int> &axb = map.getKeyVertices()[axa.m_connections[z]];
            for (int axbi : axb) {
                auto res = std::lower_bound(conn.begin(), conn.end(), axbi);
                if (res == conn.end() || axbi < *res) {
                    conn.insert(res, axbi);
                    keyvertexcounts[axbi] += 1;
                }
            }
        }
    }

    // ok, after this fairly tedious set up, we are ready to go...
    // note axradialcuts aren't required anymore...

    AxialMinimiser minimiser(map, ax_seg_cuts.size(), radialsegs.size());

    std::vector<Line> lines_s, lines_m;

    minimiser.removeSubsets(ax_seg_cuts, radialsegs, radialdivisions, mapData.radialLines,
                            keyvertexconns, keyvertexcounts);

    // make new lines here (assumes line map has only lines)
    int k = -1;
    for (auto &shape : map.getAllShapes()) {
        k++;
        if (!minimiser.removed(k)) {
            lines_s.push_back(shape.second.getLine());
        }
    }

    minimiser.fewestLongest(ax_seg_cuts, radialsegs, radialdivisions, mapData.radialLines,
                            keyvertexconns, keyvertexcounts);

    // make new lines here (assumes line map has only lines
    for (int k = 0; k < int(map.getAllShapes().size()); k++) {
        if (!minimiser.removed(k)) {
            lines_m.push_back(depthmapX::getMapAtIndex(map.getAllShapes(), k)->second.getLine());
        }
    }

    ShapeGraph fewestlinemap_subsets("Fewest-Line Map (Subsets)", ShapeMap::AXIALMAP);
    fewestlinemap_subsets.clearAll();
    fewestlinemap_subsets.init(int(lines_s.size()), mapData.polygons.getRegion());

    fewestlinemap_subsets.initialiseAttributesAxial();
    for (size_t k = 0; k < lines_s.size(); k++) {
        fewestlinemap_subsets.makeLineShape(lines_s[k]);
    }
    fewestlinemap_subsets.makeConnections();

    ShapeGraph fewestlinemap_minimal("Fewest-Line Map (Minimal)", ShapeMap::AXIALMAP);
    fewestlinemap_minimal.clearAll();
    fewestlinemap_minimal.init(
        int(lines_m.size()),
        mapData.polygons.getRegion()); // used to have a '2' for double pixel density

    fewestlinemap_minimal.initialiseAttributesAxial();
    for (size_t k = 0; k < lines_m.size(); k++) {
        fewestlinemap_minimal.makeLineShape(lines_m[k]);
    }
    fewestlinemap_minimal.makeConnections();

    return std::make_tuple(std::move(fewestlinemap_subsets), std::move(fewestlinemap_minimal));
}

void AllLine::makeDivisions(ShapeGraph &map, const std::vector<PolyConnector> &polyconnections,
                            const std::vector<RadialLine> &radiallines,
                            std::map<RadialKey, std::set<int>> &radialdivisions,
                            std::map<int, std::set<int>> &axialdividers, Communicator *comm) {
    time_t atime = 0;
    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(Communicator::NUM_RECORDS, polyconnections.size());
    }

    for (size_t i = 0; i < polyconnections.size(); i++) {
        PixelRefVector pixels = map.pixelateLine(polyconnections[i].line);
        std::vector<size_t> testedshapes;
        auto connIter = radialdivisions.find(polyconnections[i].key);
        size_t connindex = std::distance(radialdivisions.begin(), connIter);
        double tolerance = sqrt(TOLERANCE_A); // * polyconnections[i].line.length();
        for (size_t j = 0; j < pixels.size(); j++) {
            PixelRef pix = pixels[j];
            const auto &shapes = map.getShapesAtPixel(pix);
            for (const ShapeRef &shape : shapes) {
                auto iter = depthmapX::findBinary(testedshapes, shape.m_shape_ref);
                if (iter != testedshapes.end()) {
                    continue;
                }
                testedshapes.insert(iter, int(shape.m_shape_ref));
                const Line &line = map.getAllShapes().find(shape.m_shape_ref)->second.getLine();
                //
                if (intersect_region(line, polyconnections[i].line, tolerance * line.length())) {
                    switch (intersect_line_distinguish(line, polyconnections[i].line,
                                                       tolerance * line.length())) {
                    case 0:
                        break;
                    case 2: {
                        size_t index =
                            depthmapX::findIndexFromKey(axialdividers, (int)shape.m_shape_ref);
                        if (index != shape.m_shape_ref) {
                            throw 1; // for the code to work later this can't be true!
                        }
                        axialdividers[index].insert(connindex);
                        connIter->second.insert(shape.m_shape_ref);
                    } break;
                    case 1: {
                        size_t index =
                            depthmapX::findIndexFromKey(axialdividers, (int)shape.m_shape_ref);
                        if (index != shape.m_shape_ref) {
                            throw 1; // for the code to work later this can't be true!
                        }
                        //
                        // this makes sure actually crosses between the line and the
                        // openspace properly
                        if (radiallines[connindex].cuts(line)) {
                            axialdividers[index].insert(connindex);
                            connIter->second.insert(shape.m_shape_ref);
                        }
                    } break;
                    default:
                        break;
                    }
                }
            }
        }
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, i);
            }
        }
    }
}

ShapeGraph AllLine::createAllLineMap(const std::string &name) {
    return ShapeGraph(name, ShapeMap::ALLLINEMAP);
}
