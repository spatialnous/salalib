// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dxfp.h"

#include "../genlib/comm.h" // for communicator
#include "../genlib/stringutils.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

static int counter = 0;

///////////////////////////////////////////////////////////////////////////////

bool operator>(const DxfTableRow &a, const DxfTableRow &b) // for hash table
{
    return a.m_name > b.m_name;
}

bool operator<(const DxfTableRow &a, const DxfTableRow &b) // for hash table
{
    return a.m_name < b.m_name;
}

bool operator==(const DxfTableRow &a, const DxfTableRow &b) // for hash table
{
    return a.m_name == b.m_name;
}

///////////////////////////////////////////////////////////////////////////////

DxfParser::DxfParser(Communicator *comm /* = NULL */)
    : m_time(0), m_size(0), m_communicator(comm) {}

const DxfVertex &DxfParser::getExtMin() const { return m_region.getExtMin(); }

const DxfVertex &DxfParser::getExtMax() const { return m_region.getExtMax(); }

DxfLayer *DxfParser::getLayer(
    const std::string &layerName) // const <- removed as m_layers may be changed if DXF is poor
{
    std::map<std::string, DxfLayer>::iterator layerIter = m_layers.find(layerName);
    if (layerIter == m_layers.end()) {
        m_layers.insert(std::pair<std::string, DxfLayer>(layerName, DxfLayer(layerName)));
        return &(m_layers.find(layerName)->second);
    }
    return &(layerIter->second);
}

DxfLineType *DxfParser::getLineType(
    const std::string &lineTypeName) // const <- removed as m_layers may be changed if DXF is poor
{
    static DxfLineType lineType;

    lineType.m_name = lineTypeName;

    std::map<std::string, DxfLineType>::iterator lineTypeIter = m_lineTypes.find(lineTypeName);
    if (lineTypeIter == m_lineTypes.end()) {
        m_lineTypes.insert(std::pair<std::string, DxfLineType>(lineTypeName, lineType));
        return &(m_lineTypes.find(lineTypeName)->second);
    }
    return &(lineTypeIter->second);
}

size_t DxfParser::numLayers() const { return m_layers.size(); }

size_t DxfParser::numLineTypes() const { return m_lineTypes.size(); }

///////////////////////////////////////////////////////////////////////////////

std::istream &operator>>(std::istream &stream, DxfParser &dxfp) {
    if (dxfp.m_communicator) {
        auto size = static_cast<size_t>(dxfp.m_communicator->GetInfileSize());
        dxfp.m_communicator->CommPostMessage(Communicator::NUM_RECORDS, size);

        qtimer(dxfp.m_time, 0);
    }

    return dxfp.open(stream);
}

std::istream &DxfParser::open(std::istream &stream) {
    DxfToken token;
    int section = UNIDENTIFIED;

    while (!stream.eof() && section != ENDOFFILE) {
        switch (section) {
        case ZEROTOKEN:
            if (token.data == "SECTION") {
                // find out the section
                stream >> token;
                m_size += token.size;
                //
                if (token.code != 2) {
                    // oops...
                    section = UNIDENTIFIED;
                } else if (token.data == "HEADER") {
                    section = HEADER;
                } else if (token.data == "TABLES") {
                    section = TABLES;
                } else if (token.data == "BLOCKS") {
                    section = BLOCKS;
                } else if (token.data == "ENTITIES") {
                    section = ENTITIES;
                } else {
                    section = UNIDENTIFIED;
                }
            } else if (token.data == "EOF") {
                section = ENDOFFILE;
            } else {
                section = UNIDENTIFIED;
            }
            break;
        case HEADER:
            openHeader(stream);
            section = UNIDENTIFIED;
            break;
        case TABLES:
            openTables(stream);
            section = UNIDENTIFIED;
            break;
        case BLOCKS:
            openBlocks(stream);
            section = UNIDENTIFIED;
            break;
        case ENTITIES:
            openEntities(stream,
                         token); // I'm adding the token here as before the function was unsafe, but
                                 // I'm not sure reuse of this token is a good idea AT 29-APR-11
            section = UNIDENTIFIED;
            break;
        default:
            stream >> token;
            m_size += token.size;
            if (token.code == 0) {
                section = ZEROTOKEN;
            }
            break;
        }
        if (m_communicator) {
            counter++;
            if (qtimer(m_time, 500)) {
                if (m_communicator->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                m_communicator->CommPostMessage(Communicator::CURRENT_RECORD,
                                                static_cast<size_t>(m_size));
            }
        }
    }

    // Get overall bounding box from layers:
    for (const auto &layer : m_layers) {
        if (!layer.second.empty()) {
            m_region.merge(layer.second);
        }
    }
    return stream;
}

///////////////////////////////////////////////////////////////////////////////

void DxfParser::openHeader(std::istream &stream) {
    DxfToken token;
    int subsection = UNIDENTIFIED;

    DxfVertex vertex;

    while (!stream.eof() && subsection != ENDSEC) {
        switch (subsection) {
        case ZEROTOKEN:
            if (token.data == "ENDSEC") {
                subsection = ENDSEC;
            }
            /*
            // EXTMIN and EXTMAX are deprecated: Now calculate ourselves instead...
            // although now my blocks reading is done properly, should be okay!
            else if (token.data == "$EXTMIN") {
               subsection = EXTMIN;
            }
            else if (token.data == "$EXTMAX") {
               subsection = EXTMAX;
            }
            */
            else {
                subsection = UNIDENTIFIED;
            }
            break;
            /*
            // EXTMIN and EXTMAX are deprecated: Now calculate ourselves instead...
            // although now my blocks reading is done properly, should be okay!
         case EXTMIN:
            stream >> token;
            m_size += token.size;
            if ( vertex.parse(token, this) ) {
               m_extmin = vertex;
               vertex.clear(); // reuse
               subsection = ZEROTOKEN;
            }
            break;
         case EXTMAX:
            stream >> token;
            m_size += token.size;
            if ( vertex.parse(token, this) ) {
               m_extmax = vertex;
               vertex.clear(); // reuse
               subsection = ZEROTOKEN;
            }
            break;
            */
        default:
            stream >> token;
            m_size += token.size;
            if (token.code == 0 || token.code == 9) { // 9 is used as a '0' in the header
                subsection = ZEROTOKEN;
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void DxfParser::openTables(std::istream &stream) {
    DxfToken token;
    int subsection = UNIDENTIFIED;

    DxfLayer layer;
    DxfLineType lineType;

    while (!stream.eof() && subsection != ENDSEC) {
        switch (subsection) {
        case ZEROTOKEN:
            if (token.data == "TABLE") {
                // find out the table type
                stream >> token;
                m_size += token.size;
                //
                if (token.code != 2) {
                    // oops...
                    subsection = UNIDENTIFIED;
                } else if (token.data == "LTYPE") {
                    subsection = LTYPE_TABLE;
                } else if (token.data == "LAYER") {
                    subsection = LAYER_TABLE;
                } else {
                    subsection = UNIDENTIFIED;
                }
            } else if (token.data == "ENDSEC") {
                subsection = ENDSEC;
            } else {
                subsection = UNIDENTIFIED;
            }
            break;
        case LTYPE_TABLE:
            stream >> token;
            m_size += token.size;
            if (token.code == 0) {
                if (token.data == "LTYPE") {
                    subsection = LTYPE_ROW;
                } else if (token.data == "ENDTAB") {
                    subsection = ZEROTOKEN;
                }
            }
            break;
        case LTYPE_ROW:
            stream >> token;
            m_size += token.size;
            if (lineType.parse(token, this)) {
                m_lineTypes.insert(std::pair<std::string, DxfLineType>(lineType.m_name, lineType));
                if (token.data == "ENDTAB") {
                    subsection = ZEROTOKEN;
                }
            }
            break;
        case LAYER_TABLE:
            stream >> token;
            m_size += token.size;
            if (token.code == 0) {
                if (token.data == "LAYER") {
                    subsection = LAYER_ROW;
                } else if (token.data == "ENDTAB") {
                    subsection = ZEROTOKEN;
                }
            }
            break;
        case LAYER_ROW:
            stream >> token;
            m_size += token.size;
            if (layer.parse(token, this)) {
                //               m_layers.add( layer );
                if (token.data == "ENDTAB") {
                    subsection = ZEROTOKEN;
                }
            }
            break;
        default:
            stream >> token;
            m_size += token.size;
            if (token.code == 0) {
                subsection = ZEROTOKEN;
            }
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void DxfParser::openBlocks(std::istream &stream) {
    DxfToken token;
    int subsection = UNIDENTIFIED;

    DxfBlock block;

    while (!stream.eof() && subsection != ENDSEC) {
        switch (subsection) {
        case ZEROTOKEN:
            if (token.data == "BLOCK") {
                subsection = BLOCK;
            } else if (token.data == "ENDSEC") {
                subsection = ENDSEC;
            } else {
                subsection = UNIDENTIFIED;
            }
            break;
        case BLOCK:
            stream >> token;
            m_size += token.size;
            if (block.parse(token, this)) {
                m_blocks.insert(std::pair<std::string, DxfBlock>(block.m_name, block));
                if (token.data == "ENDBLK") {
                    subsection = ZEROTOKEN;
                } else {
                    // this drills down to the data for the block:
                    openEntities(stream, token, &(m_blocks[block.m_name]));
                    // only if the block ends should we move up:
                    if (token.data == "ENDBLK") {
                        subsection = ZEROTOKEN;
                    }
                }
            }
            break;
        default:
            stream >> token;
            m_size += token.size;
            if (token.code == 0) {
                subsection = ZEROTOKEN;
            }
            break;
        }
        if (m_communicator) {
            counter++;
            if (qtimer(m_time, 500)) {
                if (m_communicator->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                m_communicator->CommPostMessage(Communicator::CURRENT_RECORD,
                                                static_cast<size_t>(m_size));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

void DxfParser::openEntities(std::istream &stream, DxfToken &token, DxfBlock *block) {
    int subsection = UNIDENTIFIED;
    if (token.code == 0) {
        // a block must always pass it's first token:
        subsection = ZEROTOKEN;
    }

    DxfVertex point;
    DxfLine line;
    DxfPolyLine polyLine;
    DxfLwPolyLine lwPolyLine;
    DxfArc arc;
    DxfEllipse ellipse;
    DxfCircle circle;
    DxfSpline spline;
    DxfInsert insert;

    std::string layerName;
    std::string lineTypeName;

    while (!stream.eof() && subsection != ENDSEC) {
        switch (subsection) {
        case ZEROTOKEN:
            if (token.data == "POINT") {
                subsection = POINT;
            } else if (token.data == "LINE") {
                subsection = LINE;
            } else if (token.data == "POLYLINE") {
                subsection = POLYLINE;
            } else if (token.data == "LWPOLYLINE") {
                subsection = LWPOLYLINE;
            } else if (token.data == "ARC") {
                subsection = ARC;
            } else if (token.data == "ELLIPSE") {
                subsection = ELLIPSE;
            } else if (token.data == "CIRCLE") {
                subsection = CIRCLE;
            } else if (token.data == "SPLINE") {
                subsection = SPLINE;
            } else if (token.data == "INSERT") {
                subsection = INSERT;
            } else if (token.data == "ENDSEC" || token.data == "ENDBLK") {
                subsection = ENDSEC;
            } else {
                subsection = UNIDENTIFIED;
            }
            break;
        case POINT:
            stream >> token;
            m_size += token.size;
            if (point.parse(token, this)) {
                DxfLayer *layer = block;
                if (layer == nullptr) {
                    layer = point.m_pLayer;
                }
                layer->m_points.push_back(point);
                layer->merge(point); // <- merge bounding box
                layer->m_totalPointCount += 1;
                point.clear();
                subsection = ZEROTOKEN;
            }
            break;
        case LINE:
            stream >> token;
            m_size += token.size;
            if (line.parse(token, this)) {
                if (line.m_start != line.m_end) {
                    DxfLayer *layer = block;
                    if (layer == nullptr) {
                        layer = line.m_pLayer;
                    }
                    layer->m_lines.push_back(line);
                    layer->merge(line); // <- merge bounding box
                    layer->m_totalPointCount += 1;
                }
                line.clear();
                subsection = ZEROTOKEN;
            }
            break;
        case POLYLINE:
            stream >> token;
            m_size += token.size;
            if (polyLine.parse(token, this)) {
                if (polyLine.m_vertexCount > 0) {
                    DxfLayer *layer = block;
                    if (layer == nullptr) {
                        layer = polyLine.m_pLayer;
                    }
                    layer->m_polyLines.push_back(polyLine);
                    size_t lineCount = (polyLine.getAttributes() & DxfPolyLine::CLOSED)
                                           ? polyLine.numVertices() - 2
                                           : polyLine.numVertices() - 1;
                    layer->merge(polyLine); // <- merge bounding box
                    layer->m_totalPointCount += lineCount;
                    polyLine.clear(); // (Now reuse)
                }
                polyLine.clear(); // (Now reuse)
                subsection = ZEROTOKEN;
            }
            break;
        case LWPOLYLINE:
            stream >> token;
            m_size += token.size;
            if (lwPolyLine.parse(token, this)) {
                if (lwPolyLine.m_vertexCount > 0) {
                    DxfLayer *layer = block;
                    if (layer == nullptr) {
                        layer = lwPolyLine.m_pLayer;
                    }
                    layer->m_polyLines.push_back(lwPolyLine);
                    size_t lineCount = (lwPolyLine.getAttributes() & DxfPolyLine::CLOSED)
                                           ? lwPolyLine.numVertices() - 2
                                           : lwPolyLine.numVertices() - 1;
                    layer->merge(lwPolyLine); // <- merge bounding box
                    layer->m_totalPointCount += lineCount;
                }
                lwPolyLine.clear(); // (Now reuse)
                subsection = ZEROTOKEN;
            }
            break;
        case ARC:
            stream >> token;
            m_size += token.size;
            if (arc.parse(token, this)) {
                DxfLayer *layer = block;
                if (layer == nullptr) {
                    layer = arc.m_pLayer;
                }
                layer->m_arcs.push_back(arc);
                layer->merge(arc);
                arc.clear(); // (Now reuse)
                subsection = ZEROTOKEN;
            }
            break;
        case ELLIPSE:
            stream >> token;
            m_size += token.size;
            if (ellipse.parse(token, this)) {
                DxfLayer *layer = block;
                if (layer == nullptr) {
                    layer = ellipse.m_pLayer;
                }
                layer->m_ellipses.push_back(ellipse);
                layer->merge(ellipse);
                ellipse.clear(); // (Now reuse)
                subsection = ZEROTOKEN;
            }
            break;
        case CIRCLE:
            stream >> token;
            m_size += token.size;
            if (circle.parse(token, this)) {
                DxfLayer *layer = block;
                if (layer == nullptr) {
                    layer = circle.m_pLayer;
                }
                layer->m_circles.push_back(circle);
                layer->merge(circle);
                circle.clear(); // (Now reuse)
                subsection = ZEROTOKEN;
            }
            break;
        case SPLINE:
            stream >> token;
            m_size += token.size;
            if (spline.parse(token, this)) {
                if (spline.numVertices() > 0) {
                    DxfLayer *layer = block;
                    if (layer == nullptr) {
                        layer = spline.m_pLayer;
                    }
                    layer->m_splines.push_back(spline);
                    size_t lineCount = (spline.getAttributes() & DxfSpline::CLOSED)
                                           ? spline.numVertices() - 2
                                           : spline.numVertices() - 1;
                    layer->merge(spline);
                    layer->m_totalPointCount += lineCount;
                    spline.clear(); // (Now reuse)
                }
                subsection = ZEROTOKEN;
            }
            break;
        case INSERT:
            stream >> token;
            m_size += token.size;
            if (insert.parse(token, this)) {
                if (insert.m_blockName.length()) {
                    DxfLayer *layer = block;
                    if (layer == nullptr) {
                        layer = insert.m_pLayer;
                        // we are in the entities section, unwind all the blocks
                        layer->insert(insert, this);
                    } else {
                        // we are within a block, hold on until we load all of them
                        // before we can unwind them into the entities section
                        layer->m_inserts.push_back(insert);
                    }
                }
                insert.clear();
                subsection = ZEROTOKEN;
            }
            break;
        default:
            stream >> token;
            m_size += token.size;
            if (token.code == 0) {
                subsection = ZEROTOKEN;
            }
            break;
        }
        if (m_communicator) {
            counter++;
            if (qtimer(m_time, 500)) {
                if (m_communicator->IsCancelled()) {
                    throw Communicator::CancelledException();
                }
                m_communicator->CommPostMessage(Communicator::CURRENT_RECORD,
                                                static_cast<size_t>(m_size));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

// Individual parsing of the types

DxfTableRow::DxfTableRow(const std::string &name) : m_name(name) {}

bool DxfTableRow::parse(const DxfToken &token, DxfParser *) {
    bool parsed = false;

    switch (token.code) {
    case 2:
        m_name = token.data;
        break;
    case 0:
        parsed = true;
        break;
    default:
        break;
    }
    return parsed;
}

///////////////////////////////////////////////////////////////////////////////

DxfEntity::DxfEntity(int tag) : m_tag(tag) {}

void DxfEntity::clear() { m_tag = -1; }

bool DxfEntity::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 5:
        m_tag = std::stoi(std::string("0x") + token.data); // tag is in hex
        break;
    case 6:
        m_pLineType = parser->getLineType(token.data);
        break;
    case 8:
        m_pLayer = parser->getLayer(token.data);
        break;
    case 0:
        parsed = true;
        break;
    default:
        break;
    }
    return parsed;
}

///////////////////////////////////////////////////////////////////////////////

DxfVertex::DxfVertex(int tag) : DxfEntity(tag), x(0.0), y(0.0), z(0.0) {}

void DxfVertex::clear() {
    x = 0.0;
    y = 0.0;
    z = 0.0;

    DxfEntity::clear();
}

bool operator==(const DxfVertex &a, const DxfVertex &b) {
    return (a.x == b.x && a.y == b.y && a.z == b.z);
}

bool operator!=(const DxfVertex &a, const DxfVertex &b) {
    return (a.x != b.x || a.y != b.y || a.z != b.z);
}

bool DxfVertex::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 10:
        x = std::stod(token.data);
        break;
    case 20:
        y = std::stod(token.data);
        break;
    case 30:
        z = std::stod(token.data);
        break;
    case 0:
    case 9: // 0 is standard vertex, 9 is for header section variables
        parsed = true;
        break;
    default:
        parsed = DxfEntity::parse(token, parser); // base class parse
    }

    return parsed;
}

///////////////////////////////////////////////////////////////////////////////

DxfLine::DxfLine(int tag) : DxfEntity(tag) {}

void DxfLine::clear() {
    DxfRegion::clear();
    DxfEntity::clear();
}

bool DxfLine::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 10:
        m_start.x = std::stod(token.data);
        break;
    case 20:
        m_start.y = std::stod(token.data);
        break;
    case 30:
        m_start.z = std::stod(token.data);
        break;
    case 11:
        m_end.x = std::stod(token.data);
        break;
    case 21:
        m_end.y = std::stod(token.data);
        break;
    case 31:
        m_end.z = std::stod(token.data);
        break;
    case 0:
        add(m_start); // <- add to region
        add(m_end);   // <- add to region
        parsed = true;
        break;
    default:
        parsed = DxfEntity::parse(token, parser); // base class parse
        break;
    }
    return parsed;
}

///////////////////////////////////////////////////////////////////////////////

DxfPolyLine::DxfPolyLine(int tag) : DxfEntity(tag) { clear(); }

void DxfPolyLine::clear() {
    m_vertexCount = 0;
    m_vertices.clear();
    m_attributes = 0;

    DxfRegion::clear();
    DxfEntity::clear();
}

bool DxfPolyLine::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    static DxfVertex vertex;

    if (m_vertexCount) {
        if (vertex.parse(token, parser)) {
            add(vertex); // <- add to region
            if (m_min.x == 0) {
                std::cerr << "problem" << std::endl;
            }
            m_vertices.push_back(vertex);
            if (token.data == "VERTEX") { // Another vertex...
                m_vertexCount++;
            } else { // Should be a SEQEND
                parsed = true;
            }
        }
    } else { // parse the polyline header...
        switch (token.code) {
        case 0:
            if (token.data == "VERTEX") {
                m_vertexCount++;
            } else {
                parsed = true;
            }
            break;
        case 70:
            m_attributes = std::stoi(token.data);
            break;
        default:
            DxfEntity::parse(token, parser); // base class parse
            break;
        }
    }
    return parsed;
}

size_t DxfPolyLine::numVertices() const { return m_vertices.size(); }

const DxfVertex &DxfPolyLine::getVertex(size_t i) const { return m_vertices[i]; }

int DxfPolyLine::getAttributes() const { return m_attributes; }

///////////////////////////////////////////////////////////////////////////////

DxfLwPolyLine::DxfLwPolyLine(int tag) : DxfPolyLine(tag) { clear(); }

void DxfLwPolyLine::clear() {
    m_expectedVertexCount = 0;

    DxfPolyLine::clear();
}

bool DxfLwPolyLine::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    static DxfVertex vertex;

    switch (token.code) {
    case 0:
        // push final vertex
        if (m_vertexCount) {
            add(vertex); // <- add vertex to region
            m_vertices.push_back(vertex);
        }
        parsed = true;
        break;
    case 10:
        if (m_vertexCount) {
            // push last vertex
            add(vertex); // <- add vertex to region
            m_vertices.push_back(vertex);
        }
        m_vertexCount++;
        vertex.clear();
        vertex.parse(token, parser);
        break;
    case 20:
    case 30:
        // continue last vertex:
        vertex.parse(token, parser);
        break;
    case 70:
        m_attributes = std::stoi(token.data);
        break;
    case 90:
        m_expectedVertexCount = std::stoi(token.data);
        break;
    default:
        DxfEntity::parse(token, parser); // base class parse
        break;
    }

    return parsed;
}

///////////////////////////////////////////////////////////////////////////////

DxfArc::DxfArc(int tag) : DxfEntity(tag) {}

void DxfArc::clear() {
    m_start = 0.0;
    m_end = 0.0;

    DxfRegion::clear();
    DxfEntity::clear();
}

bool DxfArc::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 10:
        m_centre.x = std::stod(token.data);
        break;
    case 20:
        m_centre.y = std::stod(token.data);
        break;
    case 30:
        m_centre.z = std::stod(token.data);
        break;
    case 40:
        m_radius = std::stod(token.data);
        break;
    case 50:
        m_start = std::stod(token.data);
        break;
    case 51:
        m_end = std::stod(token.data);
        break;
    case 0: {
        // just loop round if m_start is bigger than m_end
        if (m_start > m_end) {
            m_end += 360;
        }
        // technically should check for arc limits for tighter bounding box,
        // but easier to give circular bounding box
        DxfVertex bounds;
        bounds.x = m_centre.x - m_radius;
        bounds.y = m_centre.y - m_radius;
        bounds.z = m_centre.z;
        add(bounds); // <- add to region
        bounds.x = m_centre.x + m_radius;
        bounds.y = m_centre.y + m_radius;
        bounds.z = m_centre.z;
        add(bounds); // <- add to region
        parsed = true;
    } break;
    default:
        parsed = DxfEntity::parse(token, parser); // base class parse
        break;
    }

    return parsed;
}

int DxfArc::numSegments(int segments) const {
    return ((m_start == m_end) ? segments : (static_cast<int>(m_end - m_start) * segments / 360));
}

DxfVertex DxfArc::getVertex(int i, int segments) const {
    DxfVertex v = m_centre;
    double range = 2.0 * DXF_PI;
    if (m_start != m_end)
        range = (m_end - m_start) * DXF_PI / 180.0;
    double ang = range * static_cast<double>(i) / static_cast<double>(segments);
    if (m_start != m_end) {
        ang += 2.0 * DXF_PI * (m_start / 360.0);
    }
    // ARCS go anticlockwise from (1 0)
    v.x = m_centre.x + m_radius * cos(ang);
    v.y = m_centre.y + m_radius * sin(ang);
    v.z = m_centre.z;
    return v;
}

void DxfArc::reflect(double x, double y) {
    if (x < 0) {
        m_start = 180 - m_start;
        m_end = 180 - m_end;
    }
    if (y < 0) {
        m_start = 360 - m_start;
        m_end = 360 - m_end;
    }
    while (m_start < 0) {
        m_start += 360;
    }
    while (m_end < 0) {
        m_end += 360;
    }
    if (x * y < 0) {
        double temp;
        temp = m_start;
        m_start = m_end;
        m_end = temp;
    }
}

DxfEllipse::DxfEllipse(int tag) : DxfEntity(tag) {}

void DxfEllipse::clear() {
    m_start = 0.0;
    m_end = 0.0;

    DxfRegion::clear();
    DxfEntity::clear();
}

bool DxfEllipse::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 10:
        m_centre.x = std::stod(token.data);
        break;
    case 20:
        m_centre.y = std::stod(token.data);
        break;
    case 30:
        m_centre.z = std::stod(token.data);
        break;
    case 11:
        m_majorAxisEndPoint.x = std::stod(token.data);
        break;
    case 21:
        m_majorAxisEndPoint.y = std::stod(token.data);
        break;
    case 31:
        m_majorAxisEndPoint.z = std::stod(token.data);
        break;
    case 210:
        m_extrusionDirection.x = std::stod(token.data);
        break;
    case 220:
        m_extrusionDirection.y = std::stod(token.data);
        break;
    case 230:
        m_extrusionDirection.z = std::stod(token.data);
        break;
    case 40:
        m_minorMajorAxisRatio = std::stod(token.data);
        break;
    case 41:
        m_start = std::stod(token.data);
        break;
    case 42:
        m_end = std::stod(token.data);
        break;
    case 0: {
        // just loop round if m_start is bigger than m_end
        if (m_start > m_end) {
            m_end += 360;
        }
        // technically should check for ellipse limits for tighter bounding box,
        // but easier to give circular bounding box
        DxfVertex bounds;
        double xdiff = fabs(m_majorAxisEndPoint.x);
        double ydiff = fabs(m_majorAxisEndPoint.y);
        bounds.x = m_centre.x - xdiff;
        bounds.y = m_centre.y - ydiff;
        bounds.z = m_centre.z;
        add(bounds); // <- add to region
        bounds.x = m_centre.x + xdiff;
        bounds.y = m_centre.y + ydiff;
        bounds.z = m_centre.z;
        add(bounds); // <- add to region
        parsed = true;
    } break;
    default:
        parsed = DxfEntity::parse(token, parser); // base class parse
        break;
    }

    return parsed;
}

int DxfEllipse::numSegments(int segments) const {
    return ((m_start == m_end) ? segments
                               : static_cast<int>((m_end - m_start) * segments / (2 * DXF_PI)));
}

DxfVertex DxfEllipse::getVertex(int i, int segments) const {
    DxfVertex v = m_centre;
    double range = 2.0 * DXF_PI;
    if (m_start != m_end)
        range = (m_end - m_start);
    double ang = m_start + range * static_cast<double>(i) / static_cast<double>(segments);

    double c = cos(ang);
    double s = sin(ang);

    double reverse = 1;
    if (m_extrusionDirection.z < 0)
        reverse = -1;

    double xnew = c * m_majorAxisEndPoint.x - m_minorMajorAxisRatio * s * m_majorAxisEndPoint.y;
    double ynew =
        c * m_majorAxisEndPoint.y + reverse * m_minorMajorAxisRatio * s * m_majorAxisEndPoint.x;

    v.x = m_centre.x + xnew;
    v.y = m_centre.y + ynew;
    v.z = m_centre.z;
    return v;
}

void DxfEllipse::reflect(double x, double y) {
    if (x < 0) {
        m_start = 180 - m_start;
        m_end = 180 - m_end;
    }
    if (y < 0) {
        m_start = 360 - m_start;
        m_end = 360 - m_end;
    }
    while (m_start < 0) {
        m_start += 360;
    }
    while (m_end < 0) {
        m_end += 360;
    }
    if (x * y < 0) {
        double temp;
        temp = m_start;
        m_start = m_end;
        m_end = temp;
    }
}

///////////////////////////////////////////////////////////////////////////

DxfCircle::DxfCircle(int tag) : DxfEntity(tag) {}

void DxfCircle::clear() {
    DxfRegion::clear();
    DxfEntity::clear();
}

bool DxfCircle::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 10:
        m_centre.x = std::stod(token.data);
        break;
    case 20:
        m_centre.y = std::stod(token.data);
        break;
    case 30:
        m_centre.z = std::stod(token.data);
        break;
    case 40:
        m_radius = std::stod(token.data);
        break;
    case 0: {
        DxfVertex bounds;
        bounds.x = m_centre.x - m_radius;
        bounds.y = m_centre.y - m_radius;
        bounds.z = m_centre.z;
        add(bounds); // <- add to region
        bounds.x = m_centre.x + m_radius;
        bounds.y = m_centre.y + m_radius;
        bounds.z = m_centre.z;
        add(bounds); // <- add to region
        parsed = true;
    } break;
    default:
        parsed = DxfEntity::parse(token, parser); // base class parse
        break;
    }

    return parsed;
}

DxfVertex DxfCircle::getVertex(int i, int segments) const {
    DxfVertex v = m_centre;
    double ang = 2.0 * DXF_PI * static_cast<double>(i) / static_cast<double>(segments);
    // CIRCLES go anticlockwise from (1 0)
    v.x = m_centre.x + m_radius * cos(ang);
    v.y = m_centre.y + m_radius * sin(ang);
    v.z = m_centre.z;
    return v;
}

void DxfCircle::reflect(double, double) {
    // reflect has no effect on a circle
}

///////////////////////////////////////////////////////////////////////////

// Spline
// n.b. currently just linear interpolation between control points -
// not good, but whatever method will have to make some sort of approximation at some point

///////////////////////////////////////////////////////////////////////////////

DxfSpline::DxfSpline(int tag) : DxfEntity(tag) { clear(); }

void DxfSpline::clear() {
    m_xyz = 0;
    m_ctrlPtCount = 0;
    m_knotCount = 0;
    m_ctrlPts.clear();
    m_knots.clear();
    m_attributes = 0;

    DxfRegion::clear();
    DxfEntity::clear();
}

bool DxfSpline::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    static DxfVertex vertex;

    switch (token.code) {
    case 0:
        parsed = true;
        break;
    case 70:
        m_attributes = std::stoi(token.data);
        break;
    case 72:
        m_knotCount = static_cast<size_t>(std::stoi(token.data));
        break;
    case 73:
        m_ctrlPtCount = static_cast<size_t>(std::stoi(token.data));
        break;
    case 40:
        m_knots.push_back(std::stod(token.data));
        break;
    case 10:
        vertex.x = std::stod(token.data);
        m_xyz |= 0x0001;
        break;
    case 20:
        vertex.y = std::stod(token.data);
        m_xyz |= 0x0010;
        break;
    case 30:
        vertex.z = std::stod(token.data);
        m_xyz |= 0x0100;
        break;
    default:
        DxfEntity::parse(token, parser); // base class parse
        break;
    }

    if (m_xyz == 0x0111) {
        add(vertex); // <- add vertex to region
        m_ctrlPts.push_back(vertex);
        m_xyz = 0;
    }

    return parsed;
}

// Note: return control points not actual points!

size_t DxfSpline::numVertices() const { return m_ctrlPts.size(); }

const DxfVertex &DxfSpline::getVertex(size_t i) const { return m_ctrlPts[i]; }

int DxfSpline::getAttributes() const { return m_attributes; }

///////////////////////////////////////////////////////////////////////////////

// note: inserts are flattened on way through

DxfInsert::DxfInsert(int tag) : DxfEntity(tag) { clear(); }

void DxfInsert::clear() {
    m_blockName = "";
    m_translation.clear();
    m_scale.clear();

    // actually default scale is 1,1,1
    m_scale.x = 1.0;
    m_scale.y = 1.0;
    m_scale.z = 1.0;

    m_rotation = 0.0;

    DxfRegion::clear();
    DxfEntity::clear();
}

bool DxfInsert::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 0:
        parsed = true;
        break;
    case 2:
        m_blockName = token.data;
        break;
    case 10:
        m_translation.x = std::stod(token.data);
        break;
    case 20:
        m_translation.y = std::stod(token.data);
        break;
    case 30:
        m_translation.z = std::stod(token.data);
        break;
    case 41:
        m_scale.x = std::stod(token.data);
        break;
    case 42:
        m_scale.y = std::stod(token.data);
        break;
    case 43:
        m_scale.z = std::stod(token.data);
        break;
    case 50:
        m_rotation = std::stod(token.data);
        break;
    default:
        DxfEntity::parse(token, parser); // base class parse
        break;
    }

    return parsed;
}

///////////////////////////////////////////////////////////////////////////////

DxfLineType::DxfLineType(const std::string &name) : DxfTableRow(name) {}

bool DxfLineType::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 0:
        parsed = true;
        break;
    default:
        parsed = DxfTableRow::parse(token, parser); // base class parse
    }
    return parsed;
}

const DxfVertex &DxfLine::getStart() const { return static_cast<const DxfVertex &>(m_start); }

const DxfVertex &DxfLine::getEnd() const { return static_cast<const DxfVertex &>(m_end); }

///////////////////////////////////////////////////////////////////////////////

DxfLayer::DxfLayer(const std::string &name) : DxfTableRow(name), m_totalPointCount(0) {}

bool DxfLayer::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 0:
        parsed = true;
        break;
    default:
        parsed = DxfTableRow::parse(token, parser); // base class parse
    }
    return parsed;
}

const DxfVertex &DxfLayer::getPoint(size_t i) const { return m_points[i]; }

const DxfLine &DxfLayer::getLine(size_t i) const { return m_lines[i]; }

const DxfPolyLine &DxfLayer::getPolyLine(size_t i) const { return m_polyLines[i]; }

const DxfArc &DxfLayer::getArc(size_t i) const { return m_arcs[i]; }

const DxfEllipse &DxfLayer::getEllipse(size_t i) const { return m_ellipses[i]; }

const DxfCircle &DxfLayer::getCircle(size_t i) const { return m_circles[i]; }

const DxfSpline &DxfLayer::getSpline(size_t i) const { return m_splines[i]; }

size_t DxfLayer::numPoints() const { return m_points.size(); }

size_t DxfLayer::numLines() const { return m_lines.size(); }

size_t DxfLayer::numPolyLines() const { return m_polyLines.size(); }

size_t DxfLayer::numArcs() const { return m_arcs.size(); }

size_t DxfLayer::numEllipses() const { return m_ellipses.size(); }

size_t DxfLayer::numCircles() const { return m_circles.size(); }

size_t DxfLayer::numSplines() const { return m_splines.size(); }

void DxfLayer::insert(DxfInsert &insert, DxfParser *parser) {
    size_t i;

    // munge in insert...
    bool scale = (insert.m_scale.x != 1.0 || insert.m_scale.y != 1.0 || insert.m_scale.z != 1.0);
    bool rotate = (insert.m_rotation != 0.0 && insert.m_rotation < 359.9999999);
    if (insert.m_rotation < 0) {
        insert.m_rotation += 360;
    }

    // lookup in blocks table
    if (!parser->m_blocks.count(insert.m_blockName)) {
        // throw exception
    }
    DxfBlock &block = parser->m_blocks[insert.m_blockName];

    // unwind deeper inserts
    for (i = 0; i < block.m_inserts.size(); i++) {
        block.insert(block.m_inserts[i], parser);
    }
    // delete inserts at this level to avoid re-inserting them
    // if the block is re-inserted
    block.m_inserts.clear();

    for (i = 0; i < block.m_lines.size(); i++) {
        m_lines.push_back(block.m_lines[i]);
        // rotate, translate, scale each line as specified in the insert
        if (scale)
            m_lines.back().scale(block.m_basePoint, insert.m_scale);
        if (rotate)
            m_lines.back().rotate(block.m_basePoint, insert.m_rotation);
        m_lines.back().translate(insert.m_translation);
        merge(m_lines.back()); // <- merge bounding box
    }
    for (i = 0; i < block.m_polyLines.size(); i++) {
        m_polyLines.push_back(block.m_polyLines[i]);
        // rotate, translate, scale each line as specified in the insert
        if (scale)
            m_polyLines.back().scale(block.m_basePoint, insert.m_scale);
        if (rotate)
            m_polyLines.back().rotate(block.m_basePoint, insert.m_rotation);
        m_polyLines.back().translate(insert.m_translation);
        merge(m_polyLines.back()); // <- merge bounding box
    }
    for (i = 0; i < block.m_arcs.size(); i++) {
        m_arcs.push_back(block.m_arcs[i]);
        // rotate, translate, scale each line as specified in the insert
        if (scale)
            m_arcs.back().scale(block.m_basePoint, insert.m_scale);
        if (rotate)
            m_arcs.back().rotate(block.m_basePoint, insert.m_rotation);
        m_arcs.back().translate(insert.m_translation);
        merge(m_arcs.back()); // <- merge bounding box
    }
    for (i = 0; i < block.m_ellipses.size(); i++) {
        m_ellipses.push_back(block.m_ellipses[i]);
        // rotate, translate, scale each line as specified in the insert
        if (scale)
            m_ellipses.back().scale(block.m_basePoint, insert.m_scale);
        if (rotate)
            m_ellipses.back().rotate(block.m_basePoint, insert.m_rotation);
        m_ellipses.back().translate(insert.m_translation);
        merge(m_ellipses.back()); // <- merge bounding box
    }
    for (i = 0; i < block.m_circles.size(); i++) {
        m_circles.push_back(block.m_circles[i]);
        // rotate, translate, scale each line as specified in the insert
        if (scale)
            m_circles.back().scale(block.m_basePoint, insert.m_scale);
        // n.b., rotate does nothing with circles
        if (rotate)
            m_circles.back().rotate(block.m_basePoint, insert.m_rotation);
        m_circles.back().translate(insert.m_translation);
        merge(m_circles.back()); // <- merge bounding box
    }
    for (i = 0; i < block.m_splines.size(); i++) {
        m_splines.push_back(block.m_splines[i]);
        // rotate, translate, scale each line as specified in the insert
        if (scale)
            m_splines.back().scale(block.m_basePoint, insert.m_scale);
        if (rotate)
            m_splines.back().rotate(block.m_basePoint, insert.m_rotation);
        m_splines.back().translate(insert.m_translation);
        merge(m_splines.back()); // <- merge bounding box
    }

    m_totalPointCount += block.m_totalPointCount;
}

///////////////////////////////////////////////////////////////////////////////

DxfBlock::DxfBlock(const std::string &name) : DxfLayer(name) {}

bool DxfBlock::parse(const DxfToken &token, DxfParser *parser) {
    bool parsed = false;

    switch (token.code) {
    case 0:
        parsed = true;
        break;
    default:
        parsed = DxfLayer::parse(token, parser); // base class parse
    }
    return parsed;
}

///////////////////////////////////////////////////////////////////////////////

DxfToken::DxfToken() : code(-1), size(0) {}

std::istream &operator>>(std::istream &stream, DxfToken &token) {
    std::string codeInputLine;
    std::getline(stream, codeInputLine);
    token.code = std::stoi(codeInputLine);
    std::string dataInputLine;
    std::getline(stream, dataInputLine);
    dXstring::ltrim(dataInputLine, '\r');
    dXstring::ltrim(dataInputLine, '\n');
    dXstring::rtrim(dataInputLine, '\r');
    dXstring::rtrim(dataInputLine, '\n');
    token.data = dataInputLine;
    token.size = codeInputLine.length() + token.data.length() +
                 2; // might be missing a few end line characters --- never mind
    return stream;
}

///////////////////////////////////////////////////////////////////////////////
