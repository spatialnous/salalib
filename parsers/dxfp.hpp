// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// DXF parser header file

#pragma once

///////////////////////////////////////////////////////////////////////////////

// DXF parser reads in DXF files
// So far very simple:
// The parser reads in vertices, lines and polylines, and stores them in the
// defined layers.  It also reads in any line types defined.

#include "../genlib/comm.hpp"
#include <cmath>
#include <map>
#include <vector>

class DxfToken;

class DxfTableRow;
class DxfEntity;

class DxfVertex;
class DxfLine;
class DxfPolyLine;
class DxfArc;
class DxfCircle;
class DxfSpline;

class DxfInsert;

class DxfLineType;
class DxfLayer;
class DxfBlock;

class DxfParser;

const double DXF_PI = 3.1415926535897932384626433832795;

///////////////////////////////////////////////////////////////////////////////

// Tokens read from file

class DxfToken {
  public:
    size_t size;
    std::string data;
    int code;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    DxfToken();
    friend std::istream &operator>>(std::istream &stream, DxfToken &token);
};

///////////////////////////////////////////////////////////////////////////////

// Table (row) base classes

class DxfTableRow {
    friend class DxfParser;

  protected:
    std::string m_name;

  public:
    DxfTableRow(const std::string &name = "");
    const std::string &getName() const { return m_name; }
    virtual ~DxfTableRow() {}

  protected:
    virtual bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr);

  public:
    // for hash table storage
    friend bool operator>(const DxfTableRow &, const DxfTableRow &);
    friend bool operator<(const DxfTableRow &, const DxfTableRow &);
    friend bool operator==(const DxfTableRow &, const DxfTableRow &);
};

// Entity base class

class DxfEntity {
    friend class DxfParser;

  protected:
    // Reference data
    DxfLineType *m_pLineType = nullptr;
    DxfLayer *m_pLayer = nullptr;
    int m_tag;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    DxfEntity(int tag = -1);
    DxfEntity(const DxfEntity &) = default;
    DxfEntity &operator=(const DxfEntity &) = default;
    DxfEntity &operator=(DxfEntity &&) = default;
    void clear(); // for reuse when parsing
    virtual ~DxfEntity() {}

  protected:
    virtual bool parse(const DxfToken &token, DxfParser *parser, Communicator *comm = nullptr);
};

// Three very simple 'entities'...

// Vertex

class DxfVertex : public DxfEntity {
    friend class DxfParser;
    friend class DxfLine;
    friend class DxfPolyLine;
    friend class DxfLwPolyLine;

  public:
    double x;
    double y;
    double z;

  public:
    DxfVertex(int tag = -1);
    void clear(); // for reuse when parsing
    // some simple manipulation
    // note, all ops are 2d...
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        x = (x - baseVertex.x) * scale.x + baseVertex.x;
        y = (y - baseVertex.y) * scale.y + baseVertex.y;
    }
    // note, rotation is 2d op, angle in degrees, ccw
    void rotate(const DxfVertex &baseVertex, double angle) {
        DxfVertex reg;
        double ang = (2.0 * DXF_PI * angle / 360.0);
        reg.x = (x - baseVertex.x) * cos(ang) - (y - baseVertex.y) * sin(ang);
        reg.y = (y - baseVertex.y) * cos(ang) + (x - baseVertex.x) * sin(ang);
        x = reg.x + baseVertex.x;
        y = reg.y + baseVertex.y;
    }
    void translate(const DxfVertex &translation) {
        x += translation.x;
        y += translation.y;
    }
    //
    friend bool operator==(const DxfVertex &a, const DxfVertex &b);
    friend bool operator!=(const DxfVertex &a, const DxfVertex &b);

  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

///////////////////////////////////////////////////////////////////////////////

// Helper: a bounding box region (only 2D at present)

class DxfRegion {
  protected:
    bool m_first;

  private:
    [[maybe_unused]] unsigned _padding0 : 3 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  protected:
    DxfVertex m_min;
    DxfVertex m_max;

  public:
    DxfRegion() : m_first(true), _padding0(0), _padding1(0), m_min(), m_max() {}
    void add(const DxfVertex &v) {
        if (m_first) {
            m_min = v;
            m_max = v;
            m_first = false;
        }
        if (v.x < m_min.x)
            m_min.x = v.x;
        if (v.x > m_max.x)
            m_max.x = v.x;
        if (v.y < m_min.y)
            m_min.y = v.y;
        if (v.y > m_max.y)
            m_max.y = v.y;
    }
    void merge(const DxfVertex &point) { add(point); }
    void merge(const DxfRegion &region) {
        add(region.m_min);
        add(region.m_max);
    }
    const DxfVertex &getExtMin() const { return m_min; }
    const DxfVertex &getExtMax() const { return m_max; }
    void clear() { m_first = true; }
    bool empty() const { return m_first; }
    //
    // some simple manipulations
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        m_min.scale(baseVertex, scale);
        m_max.scale(baseVertex, scale);
    }
    // rotate tricky...
    void rotate(const DxfVertex &, double) {}
    void translate(const DxfVertex &translation) {
        m_min.translate(translation);
        m_max.translate(translation);
    }
};

///////////////////////////////////////////////////////////////////////////////

// Line

class DxfLine : public DxfEntity, public DxfRegion {
    friend class DxfParser;

  protected:
    DxfVertex m_start;
    DxfVertex m_end;

  public:
    DxfLine(int tag = -1);
    void clear(); // for reuse when parsing
    //
    const DxfVertex &getStart() const;
    const DxfVertex &getEnd() const;
    //
    // some basic manipulation
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        m_start.scale(baseVertex, scale);
        m_end.scale(baseVertex, scale);
        DxfRegion::scale(baseVertex, scale);
    }
    void rotate(const DxfVertex &baseVertex, double angle) {
        m_start.rotate(baseVertex, angle);
        m_end.rotate(baseVertex, angle);
        DxfRegion::rotate(baseVertex, angle);
    }
    void translate(const DxfVertex &translation) {
        m_start.translate(translation);
        m_end.translate(translation);
        DxfRegion::translate(translation);
    }
    //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

// PolyLine

class DxfPolyLine : public DxfEntity, public DxfRegion {
    friend class DxfParser;

  public:
    enum { CLOSED = 1 }; // CLOSED = closed polygon
  protected:
    int m_attributes;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  protected:
    size_t m_vertexCount;
    std::vector<DxfVertex> m_vertices;

  public:
    DxfPolyLine(int tag = -1);
    void clear(); // for reuse when parsing
    //
    size_t numVertices() const;
    const DxfVertex &getVertex(size_t i) const;
    int getAttributes() const;
    const DxfRegion &getBoundingBox();
    //
    // some basic manipulation
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        for (size_t i = 0; i < m_vertexCount; i++)
            m_vertices[i].scale(baseVertex, scale);
        DxfRegion::scale(baseVertex, scale);
    }
    void rotate(const DxfVertex &baseVertex, double angle) {
        for (size_t i = 0; i < m_vertexCount; i++)
            m_vertices[i].rotate(baseVertex, angle);
        DxfRegion::rotate(baseVertex, angle);
    }
    void translate(const DxfVertex &translation) {
        for (size_t i = 0; i < m_vertexCount; i++)
            m_vertices[i].translate(translation);
        DxfRegion::translate(translation);
    }
    //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator *comm = nullptr) override;
};

// LwPolyLine --- just inherit from DxfPolyLine

class DxfLwPolyLine : public DxfPolyLine {
    friend class DxfParser;

  protected:
    int m_expectedVertexCount;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public:
    DxfLwPolyLine(int tag = -1);
    void clear(); // for reuse when parsing
                  //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

///////////////////////////////////////////////////////////////////////////////

// Arcs and Cicles

class DxfArc : public DxfEntity, public DxfRegion {
    friend class DxfParser;
    DxfVertex m_centre;
    double m_radius = -1;
    mutable double m_start = -1;
    double m_end = -1;

  public:
    DxfArc(int tag = -1);
    void clear(); // for reuse when parsing
    // getVertex splits into number of segments
    int numSegments(int segments) const;
    DxfVertex getVertex(int i, int segments) const;
    const DxfVertex &getCentre() const { return m_centre; }
    const double &getRadius() const { return m_radius; }
    int getAttributes() const;
    const DxfRegion &getBoundingBox();
    //
    // some basic manipulation
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        m_centre.scale(baseVertex, scale);
        m_radius *= (fabs(scale.x) + fabs(scale.y)) / 2.0;
        // this is rather tricky to do, need to think more than just reflect around 0,0,0
        if (m_start != m_end && (scale.x < 0 || scale.y < 0)) {
            reflect(scale.x, scale.y);
        }
        DxfRegion::scale(baseVertex, scale);
    }
    void reflect(double x, double y);
    void rotate(const DxfVertex &baseVertex, double angle) {
        m_centre.rotate(baseVertex, angle);
        // this is rather tricky to do, need to think more than just rotate around 0,0,0
        if (m_start != m_end) {
            m_start += angle;
            m_end += angle;
        }
        DxfRegion::rotate(baseVertex, angle);
    }
    void translate(const DxfVertex &translation) {
        m_centre.translate(translation);
        DxfRegion::translate(translation);
    }
    //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

class DxfEllipse : public DxfEntity, public DxfRegion {
    friend class DxfParser;
    DxfVertex m_centre;
    DxfVertex m_majorAxisEndPoint;
    DxfVertex m_extrusionDirection;
    double m_minorMajorAxisRatio = 1;
    mutable double m_start = -1;
    double m_end = -1;

  public:
    DxfEllipse(int tag = -1);
    void clear(); // for reuse when parsing
    // getVertex splits into number of segments
    int numSegments(int segments) const;
    DxfVertex getVertex(int i, int segments) const;
    const DxfVertex &getCentre() const { return m_centre; }
    const double &getMinorMajorAxisRatio() const { return m_minorMajorAxisRatio; }
    int getAttributes() const;
    const DxfRegion &getBoundingBox();
    //
    // some basic manipulation
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        m_centre.scale(baseVertex, scale);

        m_majorAxisEndPoint.x *= scale.x;
        m_majorAxisEndPoint.y *= scale.y;

        // this is rather tricky to do, need to think more than just reflect around 0,0,0
        if (m_start != m_end && (scale.x < 0 || scale.y < 0)) {
            reflect(scale.x, scale.y);
        }
        DxfRegion::scale(baseVertex, scale);
    }
    void reflect(double x, double y);
    void rotate(const DxfVertex &baseVertex, double angle) {
        m_centre.rotate(baseVertex, angle);
        // this is rather tricky to do, need to think more than just rotate around 0,0,0
        if (m_start != m_end) {
            m_start += angle;
            m_end += angle;
        }
        DxfRegion::rotate(baseVertex, angle);
    }
    void translate(const DxfVertex &translation) {
        m_centre.translate(translation);
        DxfRegion::translate(translation);
    }
    //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

class DxfCircle : public DxfEntity, public DxfRegion {
    friend class DxfParser;
    DxfVertex m_centre;
    double m_radius = -1;

  public:
    DxfCircle(int tag = -1);
    void clear(); // for reuse when parsing
    DxfVertex getVertex(int i, int segments) const;
    const DxfVertex &getCentre() const { return m_centre; }
    const double &getRadius() const { return m_radius; }
    int getAttributes() const;
    const DxfRegion &getBoundingBox();
    //
    // some basic manipulation
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        m_centre.scale(baseVertex, scale);
        m_radius *= (fabs(scale.x) + fabs(scale.y)) / 2.0;
        DxfRegion::scale(baseVertex, scale);
    }
    void reflect(double x, double y);
    void rotate(const DxfVertex &baseVertex, double angle) { DxfRegion::rotate(baseVertex, angle); }
    void translate(const DxfVertex &translation) {
        m_centre.translate(translation);
        DxfRegion::translate(translation);
    }
    //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

///////////////////////////////////////////////////////////////////////////////

// Spline
// n.b. currently just linear interpolation between control points -
// not good, but whatever method will have to make some sort of approximation at some point

class DxfSpline : public DxfEntity, public DxfRegion {
    friend class DxfParser;

  public:
    enum { CLOSED = 1 }; // CLOSED = closed spline
  protected:
    int m_xyz;
    int m_attributes;
    size_t m_ctrlPtCount;
    size_t m_knotCount;
    std::vector<DxfVertex> m_ctrlPts;
    std::vector<double> m_knots;

  public:
    DxfSpline(int tag = -1);
    void clear(); // for reuse when parsing
    //
    size_t numVertices() const;
    const DxfVertex &getVertex(size_t i) const;
    int getAttributes() const;
    //
    // some basic manipulation
    void scale(const DxfVertex &baseVertex, const DxfVertex &scale) {
        for (size_t i = 0; i < m_ctrlPtCount; i++)
            m_ctrlPts[i].scale(baseVertex, scale);
        DxfRegion::scale(baseVertex, scale);
    }
    void rotate(const DxfVertex &baseVertex, double angle) {
        for (size_t i = 0; i < m_ctrlPtCount; i++)
            m_ctrlPts[i].rotate(baseVertex, angle);
        DxfRegion::rotate(baseVertex, angle);
    }
    void translate(const DxfVertex &translation) {
        for (size_t i = 0; i < m_ctrlPtCount; i++)
            m_ctrlPts[i].translate(translation);
        DxfRegion::translate(translation);
    }

  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

///////////////////////////////////////////////////////////////////////////////

// Inserts... these are flattened at parse-time and not retained in layers

class DxfInsert : public DxfEntity, public DxfRegion {
    friend class DxfParser;
    friend class DxfLayer;

  protected:
    std::string m_blockName;
    DxfVertex m_translation;
    DxfVertex m_scale;
    double m_rotation;

  public:
    DxfInsert(int tag = -1);
    void clear(); // for reuse when parsing
                  //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

///////////////////////////////////////////////////////////////////////////////

// Two very simple 'table' entries:

// Line types

class DxfLineType : public DxfTableRow {
    friend class DxfParser;

  public:
    DxfLineType(const std::string &name = "");

  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

// Layers

class DxfLayer : public DxfTableRow, public DxfRegion {
    friend class DxfParser;

  protected:
    // Originally was going to be clever, but it's far easier to have a list for each type:
    std::vector<DxfVertex> m_points;
    std::vector<DxfLine> m_lines;
    std::vector<DxfPolyLine> m_polyLines;
    std::vector<DxfArc> m_arcs;
    std::vector<DxfEllipse> m_ellipses;
    std::vector<DxfCircle> m_circles;
    std::vector<DxfSpline> m_splines;
    std::vector<DxfInsert> m_inserts;
    size_t m_totalPointCount = 0;
    size_t m_totalLineCount = 0;

  public:
    DxfLayer(const std::string &name = "");
    //
    const DxfVertex &getPoint(size_t i) const;
    const DxfLine &getLine(size_t i) const;
    const DxfPolyLine &getPolyLine(size_t i) const;
    const DxfArc &getArc(size_t i) const;
    const DxfEllipse &getEllipse(size_t i) const;
    const DxfCircle &getCircle(size_t i) const;
    const DxfSpline &getSpline(size_t i) const;
    //
    size_t numPoints() const;
    size_t numLines() const;
    size_t numPolyLines() const;
    size_t numArcs() const;
    size_t numEllipses() const;
    size_t numCircles() const;
    size_t numSplines() const;
    //
    size_t numTotalPoints() const { return m_totalPointCount; }
    size_t numTotalLines() const { return m_totalLineCount; }
    //
    // this merges an insert (so the insert remains flattened)
    void insert(DxfInsert &insert, DxfParser *parser);

  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

class DxfBlock : public DxfLayer {
    friend class DxfParser;
    friend class DxfLayer;

  protected:
    DxfVertex m_basePoint;

  public:
    DxfBlock(const std::string &name = "");
    //
  protected:
    bool parse(const DxfToken &token, DxfParser *parser, Communicator * = nullptr) override;
};

///////////////////////////////////////////////////////////////////////////////

class Communicator;

class DxfParser {
    friend class DxfInsert;
    friend class DxfLayer;

  public:
    enum token_t { UNIDENTIFIED = -2, ZEROTOKEN = -1 };
    enum section_t { HEADER, CLASSES, TABLES, BLOCKS, ENTITIES, OBJECTS, ENDOFFILE };
    enum subsection_t {
        EXTMIN,
        EXTMAX,
        LTYPE_TABLE,
        LTYPE_ROW,
        LAYER_TABLE,
        LAYER_ROW,
        BLOCK,
        POINT,
        LINE,
        POLYLINE,
        LWPOLYLINE,
        ARC,
        ELLIPSE,
        CIRCLE,
        SPLINE,
        INSERT,
        VERTEX,
        ENDSEC
    };

  protected:
    time_t m_time;

  protected:
    DxfRegion m_region;
    std::map<std::string, DxfLayer> m_layers;
    std::map<std::string, DxfBlock> m_blocks;
    std::map<std::string, DxfLineType> m_lineTypes;
    //
    size_t m_size;
    Communicator *m_communicator;

  public:
    DxfParser(Communicator *comm = nullptr);
    DxfParser(const DxfParser &) = default;
    DxfParser &operator=(const DxfParser &) = default;
    //
    std::istream &open(std::istream &stream);
    //
    void openHeader(std::istream &stream);
    void openTables(std::istream &stream);
    void openBlocks(std::istream &stream);
    void
    openEntities(std::istream &stream, DxfToken &token, DxfBlock *block = nullptr,
                 Communicator *comm = nullptr); // cannot have a default token: it's a reference.
                                                // Removed default to DxfToken() AT 29.04.11
    //
    const DxfVertex &getExtMin() const;
    const DxfVertex &getExtMax() const;
    DxfLayer *getLayer(const std::string &layerName); // const; <- removed as will have to add
                                                      // layer when DXF hasn't declared one
    DxfLineType *getLineType(const std::string &lineTypeName); // const;
    //
    size_t numLayers() const;
    size_t numLineTypes() const;
    //
    friend std::istream &operator>>(std::istream &stream, DxfParser &dxfp);

    std::map<std::string, DxfLayer> getLayers() { return m_layers; }
};

///////////////////////////////////////////////////////////////////////////////
