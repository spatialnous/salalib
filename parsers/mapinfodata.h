// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../attributetable.h"
#include "../layermanagerimpl.h"

#include "../genlib/p2dpoly.h"

#include <istream>
#include <ostream>
#include <string>

// imported and exported data
// note: this is very basic and designed for axial line import / export only

// MapInfoData is stored with axial map data

class ShapeMap;
class PointMap;
class AttributeTable;

class MapInfoData {
    friend class ShapeGraph;
    friend class ShapeGraphs;
    friend class ShapeMap;
    //
  protected:
    std::string m_version;
    std::string m_charset;
    char m_delimiter;
    std::string m_index;
    std::string m_coordsys;
    std::string m_bounds;
    //
    // no longer use columnheads and table
    // -- where possible, added directly to the data
    // pvecstring m_columnheads; // <- original mapinfo column headers
    // pvecstring m_table;       // <- original mapinfo table (stored as a flat text file!)
    //
  public:
    MapInfoData();
    //
    int import(std::istream &miffile, std::istream &midfile, ShapeMap &map);
    // bool exportFile(ostream& miffile, ostream& midfile, const ShapeGraph& map);   // n.b.,
    // deprecated: use shapemap instead
    bool exportFile(std::ostream &miffile, std::ostream &midfile, const PointMap &points);
    bool exportFile(std::ostream &miffile, std::ostream &midfile, const ShapeMap &map);
    bool exportPolygons(std::ostream &miffile, std::ostream &midfile,
                        const std::vector<std::vector<Point2f>> &polygons, const QtRegion &region);
    //
    bool readheader(std::istream &miffile);
    bool readcolumnheaders(std::istream &miffile, std::vector<std::string> &columnheads);
    void writeheader(std::ostream &miffile);
    void writetable(std::ostream &miffile, std::ostream &midfile, const AttributeTable &attributes,
                    const LayerManagerImpl layers);
    //
    std::istream &read(std::istream &stream);
    std::ostream &write(std::ostream &stream) const;
};
