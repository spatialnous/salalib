// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2018 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Quick OS land-line NTF parser

#include "ntfp.h"

#include "../genlib/comm.h" // for communicator
#include "../genlib/containerutils.h"
#include "../genlib/stringutils.h"

#include <fstream>
#include <iostream>
#include <sstream>

///////////////////////////////////////////////////////////////////////////////

int NtfPoint::parse(const std::string &token, bool secondhalf /* = false */) {
    if (secondhalf) {
        std::string second = token.substr(0, static_cast<size_t>(chars));
        b = stoi(second);
        if (chars == 5) {
            b *= 100;
        }
        return 2;
    } else if (static_cast<int>(token.length()) < chars * 2) {
        if (static_cast<int>(token.length()) < chars) {
            return 0;
        }
        std::string first = token.substr(0, static_cast<size_t>(chars));
        a = stoi(first);
        if (chars == 5) {
            a *= 100;
        }
        return 1;
    } else {
        std::string first = token.substr(0, static_cast<size_t>(chars));
        std::string second = token.substr(static_cast<size_t>(chars), static_cast<size_t>(chars));
        a = stoi(first);
        b = stoi(second);
        if (chars == 5) {
            a *= 100;
            b *= 100;
        }
    }
    return 2;
}

void NtfMap::fitBounds(const Line4f &li) {
    if (m_region.atZero()) {
        m_region = li;
    } else {
        m_region = m_region.runion(li);
    }
}

void NtfMap::addGeom(size_t layerIdx, NtfGeometry &geom) {
    m_lineCount += static_cast<int>(geom.lines.size());
    layers[layerIdx].m_lineCount += geom.lines.size();
    layers[layerIdx].geometries.push_back(geom);
    for (size_t i = 0; i < geom.lines.size(); i++) {
        fitBounds(geom.lines[i]);
    }
    geom.lines.clear();
}

///////////////////////////////////////////////////////////////////////////////

Line4f NtfMap::makeLine(const NtfPoint &a, const NtfPoint &b) {
    // In future requires offset
    return Line4f(Point2f(static_cast<double>(m_offset.a) + static_cast<double>(a.a) / 100.0,
                          static_cast<double>(m_offset.b) + static_cast<double>(a.b) / 100.0),
                  Point2f(static_cast<double>(m_offset.a) + static_cast<double>(b.a) / 100.0,
                          static_cast<double>(m_offset.b) + static_cast<double>(b.b) / 100.0));
}

void NtfMap::open(const std::vector<std::string> &fileset, Communicator *comm) {
    time_t time = 0;
    qtimer(time, 0);

    std::vector<int> featcodes;
    /*
       m_bottomLeft.a =  2147483647;   // 2^31 - 1
       m_bottomLeft.b =  2147483647;
       m_topRight.a   = -2147483647;
       m_topRight.b   = -2147483647;
    */
    m_lineCount = 0;
    layers.clear();

    for (size_t i = 0; i < fileset.size(); i++) {

        std::ifstream stream(fileset[i].c_str());

        int filetype = NTF_UNKNOWN;

        while (!stream.eof() && filetype == NTF_UNKNOWN) {
            std::string line;
            dXstring::safeGetline(stream, line);
            if (line.length() > 2) {
                if (dXstring::beginsWith<std::string>(line, "02")) {
                    dXstring::toLower(line);
                    if (dXstring::beginsWith<std::string>(line, "02land-line")) {
                        filetype = NTF_LANDLINE;
                    } else if (dXstring::beginsWith<std::string>(line, "02meridian")) {
                        filetype = NTF_MERIDIAN;
                    }
                }
            }
        }

        int precision = 10;

        if (filetype == NTF_UNKNOWN) {
            // not recognised -- really ought to throw error
            stream.close();
            continue;
        } else if (filetype == NTF_LANDLINE) {
            precision = 6;
        } else if (filetype == NTF_MERIDIAN) {
            precision = 5;
        }

        NtfGeometry geom;
        NtfPoint lastpoint(precision), currpoint(precision);
        int parsing = 0;
        std::vector<int>::iterator currpos;
        int currtoken = 0;
        std::vector<std::string> tokens;

        while (!stream.eof()) {
            std::string line;
            dXstring::safeGetline(stream, line);

            if (line.length()) {
                if (parsing == 0 && dXstring::beginsWith<std::string>(line, "07")) {
                    // Grab the easting and northing offset
                    std::string easting = line.substr(46, 10);
                    std::string northing = line.substr(56, 10);
                    m_offset.a = stoi(easting);
                    m_offset.b = stoi(northing);
                }
                if (parsing == 0 && dXstring::beginsWith<std::string>(line, "05")) {
                    // Grab the feature codes
                    // Example without continuation:
                    // 050001                              Building outline\0%
                    // Example with continuation:
                    // 050001                              Building ou1%
                    // tline\0%
                    std::stringstream fullLine;
                    fullLine << line;
                    while (line.substr(line.length() - 2, 2) == "1%") {
                        // the last line had 1% so remove it
                        fullLine.seekp(-2, std::ios_base::end);
                        dXstring::safeGetline(stream, line);
                        fullLine << line;
                    }
                    line = fullLine.str();
                    line = line.substr(0, line.length() - 3);
                    std::string code = line.substr(2, 4);
                    std::string name = line.substr(36);
                    if (depthmapX::addIfNotExists(featcodes, stoi(code)))
                        layers.push_back(NtfLayer(name));
                }
                if (parsing == 0 && dXstring::beginsWith<std::string>(line, "23")) {
                    geom.lines.clear();
                    // In Landline, check to see if it's a code we recognise:
                    if (filetype == NTF_LANDLINE) {
                        std::string featcodestr = line.substr(16, 4);
                        auto pos = std::find(featcodes.begin(), featcodes.end(), stoi(featcodestr));
                        if (pos != featcodes.end()) {
                            layers[static_cast<size_t>(std::distance(featcodes.begin(), pos))]
                                .geometries.push_back(NtfGeometry());
                            parsing = 1;
                            currpos = pos;
                        }
                    } else if (filetype == NTF_MERIDIAN) {
                        // In Meridian, irritatingly the feature code *follows* the geometry,
                        // just have to read in
                        parsing = 1;
                    }
                } else if (parsing == 1) {
                    if (dXstring::beginsWith<std::string>(line, "21")) {
                        tokens.clear();
                        // Some line data:
                        // read to end, and possibly leave hanging:
                        tokens = dXstring::split(line, ' ', true);
                        tokens[0] = tokens[0].substr(13);
                        lastpoint.parse(tokens[0]);
                        currtoken = 1;
                        parsing = 3;
                    }
                } else if (parsing > 1) {
                    if (dXstring::beginsWith<std::string>(line, "00")) {
                        tokens = dXstring::split(line, ' ', true);
                        tokens[0] = tokens[0].substr(2);
                        currtoken = 0;
                    } else if (dXstring::beginsWith<std::string>(line, "14") &&
                               filetype == NTF_MERIDIAN) {
                        // Meridian record for this line:
                        // finish up and add if featcode is recognised
                        // (goodness knows how we are supposed to know in advance what sort of
                        // feature we are given)
                        if (line.length() > 25 && line.substr(23, 2) == "FC") {
                            std::string featcodestr = line.substr(25, 4);
                            auto pos =
                                std::find(featcodes.begin(), featcodes.end(), stoi(featcodestr));
                            if (pos != featcodes.end()) {
                                addGeom(static_cast<size_t>(std::distance(featcodes.begin(), pos)),
                                        geom);
                            }
                        }
                        parsing = 0;
                    }
                }
                if (parsing > 1) {
                    if (parsing == 2) { // hanging half point:
                        currpoint.parse(tokens[0], true);
                        Line4f li = makeLine(lastpoint, currpoint);
                        geom.lines.push_back(li);
                        lastpoint = currpoint;
                        currtoken = 1;
                    }
                    for (size_t j = static_cast<size_t>(currtoken); j < tokens.size(); j++) {
                        int numbersparsed = currpoint.parse(tokens[j]);
                        if (numbersparsed == 2) {
                            Line4f li = makeLine(lastpoint, currpoint);
                            geom.lines.push_back(li);
                            lastpoint = currpoint;
                        } else if (numbersparsed == 1) {
                            parsing = 2; // hanging half point
                        } else {
                            parsing = 3;
                        }
                    }
                    if (tokens.back()[tokens.back().length() - 2] ==
                        '0') { // 0 here indicates no continuation
                        if (filetype == NTF_LANDLINE) {
                            addGeom(static_cast<size_t>(std::distance(featcodes.begin(), currpos)),
                                    geom);
                            parsing = 0;
                        }
                    }
                }
            }
            if (comm) {
                if (qtimer(time, 500)) {
                    if (comm->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                }
            }
        }
    }
}
