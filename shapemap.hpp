// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

#pragma once

#include "attributemap.hpp"
#include "attributetable.hpp"
#include "attributetablehelpers.hpp"
#include "attributetableview.hpp"
#include "connector.hpp"
#include "importtypedefs.hpp"
#include "layermanagerimpl.hpp"
#include "parsers/mapinfodata.hpp"
#include "salaevent.hpp"
#include "salashape.hpp"
#include "shaperef.hpp"
#include "spacepix.hpp"

#include "genlib/containerutils.hpp"
#include "genlib/simpleline.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

class ShapeMap : public AttributeMap {

  public:
    // now shapemaps cover a multitude of different types, record here:
    // (note, allline maps are automatically generated and have extra information recorded for line
    // reduction) Do not change numeric values!  They are saved to file. Note the Pesh map does
    // auto-overlap of shape-shape (yet...), so can be used for an arbitrary shape map
    enum {
        EMPTYMAP = 0x0000,
        DRAWINGMAP = 0x0001,
        DATAMAP = 0x0002,
        POINTMAP = 0x0004,
        CONVEXMAP = 0x0008,
        ALLLINEMAP = 0x0010,
        AXIALMAP = 0x0020,
        SEGMENTMAP = 0x0040,
        PESHMAP = 0x0080,
        LINEMAP = 0x0070
    };
    enum {
        COPY_NAME = 0x0001,
        COPY_GEOMETRY = 0x0002,
        COPY_ATTRIBUTES = 0x0004,
        COPY_GRAPH = 0x0008,
        COPY_ALL = 0x000f
    };

  protected:
    std::string m_name;
    int m_mapType = EMPTYMAP;
    // counters
    int m_objRef = -1;
    //
    // quick grab for shapes
    depthmapX::ColumnMatrix<std::vector<ShapeRef>> m_pixelShapes; // i rows of j columns
    //
    std::map<int, SalaShape> m_shapes;
    //
    std::vector<SalaEvent> m_undobuffer;
    //
    // for graph functionality
    // Note: this list is stored PACKED for optimal performance on graph analysis
    // ALWAYS check it is in the same order as the shape list and attribute table
    std::vector<Connector> m_connectors;
    //
    // for geometric operations
    double m_tolerance;

    // links and unlinks
    std::vector<OrderedSizeTPair> m_links;
    std::vector<OrderedSizeTPair> m_unlinks;

    MapInfoData m_mapinfodata;
    bool m_hasMapInfoData = false;

    bool m_hasgraph = false;

  private:
    [[maybe_unused]] unsigned _padding0 : 2 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  public:
    void moveData(ShapeMap &other) {
        m_shapes = std::move(other.m_shapes);
        m_connectors = std::move(other.m_connectors);
        m_links = std::move(other.m_links);
        m_unlinks = std::move(other.m_unlinks);
        m_mapinfodata = std::move(other.m_mapinfodata);
        m_hasMapInfoData = other.m_hasMapInfoData;
        m_rows = other.m_rows;
        m_cols = other.m_cols;
        m_region = std::move(other.m_region);
        m_mapType = other.m_mapType;
        m_hasgraph = other.m_hasgraph;
    }

  public:
    ShapeMap(const std::string &name = std::string(), int type = EMPTYMAP);

    // TODO: copyMapType is currently set to false, because previous versions
    // of the library assume this, and some regression tests fail. Update
    // on next regression baseline executabled
    void copy(const ShapeMap &shapemap, int copyflags = 0, bool copyMapType = false);

    ShapeMap(ShapeMap &&other)
        : AttributeMap(std::move(other.m_attributes), std::move(other.m_attribHandle),
                       std::move(other.m_layers)),
          m_name(std::move(other.m_name)), m_pixelShapes(std::move(other.m_pixelShapes)) {
        moveData(other);
    }
    ShapeMap &operator=(ShapeMap &&other) {
        m_name = std::move(other.m_name);
        m_pixelShapes = std::move(other.m_pixelShapes);
        m_attributes = std::move(other.m_attributes);
        m_attribHandle = std::move(other.m_attribHandle);
        m_layers = std::move(other.m_layers);
        moveData(other);
        return *this;
    }
    ShapeMap(const ShapeMap &) = delete;
    ShapeMap &operator=(const ShapeMap &other) = delete;

    ~ShapeMap() override = default;

    // TODO: These three functions should be refactored out of the code as much as possible
    // they are only left here because they're being used by various components that still
    // access the attribute table through indices. Once these are removed these functions
    // should only appear sparingly or removed entirely. The bits of the application
    // that still use them are the connections of the axial/segment maps and the point
    // in polygon functions.
    const std::map<int, SalaShape>::const_iterator getShapeRefFromIndex(size_t index) const {
        return depthmapX::getMapAtIndex(m_shapes, index);
    }
    AttributeRow &getAttributeRowFromShapeIndex(size_t index) {
        return m_attributes->getRow(AttributeKey(getShapeRefFromIndex(index)->first));
    }
    const AttributeRow &getAttributeRowFromShapeIndex(size_t index) const {
        return m_attributes->getRow(AttributeKey(getShapeRefFromIndex(index)->first));
    }

    void clearAll();
    // num shapes total
    size_t getShapeCount() const { return m_shapes.size(); }
    // num shapes for this object (note, request by object rowid
    // -- on interrogation, this is what you will usually receive)
    size_t getShapeCount(size_t rowid) const {
        return depthmapX::getMapAtIndex(m_shapes, rowid)->second.points.size();
    }
    //
    int getIndex(size_t rowid) const { return depthmapX::getMapAtIndex(m_shapes, rowid)->first; }
    //
    // add shape tools
    void makePolyPixels(int shaperef);
    void shapePixelBorder(std::map<int, int> &relations, int shaperef, int side, PixelRef currpix,
                          PixelRef minpix, bool first);
    // remove shape tools
    void removePolyPixels(int shaperef);
    //
    //
    void init(size_t size, const Region4f &r);
    int getNextShapeKey();
    // convert a single point into a shape
    int makePointShapeWithRef(
        const Point2f &point, int shapeRef, bool tempshape = false,
        const std::map<size_t, float> &extraAttributes = std::map<size_t, float>());
    int makePointShape(const Point2f &point, bool tempshape = false,
                       const std::map<size_t, float> &extraAttributes = std::map<size_t, float>());
    // or a single line into a shape
    int makeLineShapeWithRef(
        const Line4f &line, int shapeRef, bool throughUi = false, bool tempshape = false,
        const std::map<size_t, float> &extraAttributes = std::map<size_t, float>());
    int makeLineShape(const Line4f &line, bool throughUi = false, bool tempshape = false,
                      const std::map<size_t, float> &extraAttributes = std::map<size_t, float>());
    // or a polygon into a shape
    int makePolyShapeWithRef(
        const std::vector<Point2f> &points, bool open, int shapeRef, bool tempshape = false,
        const std::map<size_t, float> &extraAttributes = std::map<size_t, float>());
    int makePolyShape(const std::vector<Point2f> &points, bool open, bool tempshape = false,
                      const std::map<size_t, float> &extraAttributes = std::map<size_t, float>());

    bool hasGraph() const { return m_hasgraph; }

  public:
    // or make a shape from a shape
    int makeShape(const SalaShape &shape, int overrideShapeRef = -1,
                  const std::map<size_t, float> &extraAttributes = std::map<size_t, float>());
    // convert points to polygons
    bool convertPointsToPolys(double polyRadius,
                              std::optional<std::reference_wrapper<const std::set<int>>> selSet);
    // convert a selected pixels to a layer object (note, uses selection attribute on pixel, you
    // must select to make this work):
    int makeShapeFromPointSet(const PointMap &pointmap, const std::set<int> &selSet);
    //
    // move a shape (currently only a line shape) -- in the future should use SalaShape
    bool moveShape(int shaperef, const Line4f &line, bool undoing = false);
    // delete a shape
    void removeShape(int shaperef, bool undoing = false);
    //
    void setShapeAttributes(int rowid, const SalaShape &shape);
    //
    // some UI polygon creation tools:
    int polyBegin(const Line4f &line);
    bool polyAppend(int shapeRef, const Point2f &point);
    bool polyClose(int shapeRef);
    bool polyCancel(int shapeRef);
    // some shape creation tools for the scripting language or DLL interface
  public:
    bool canUndo() const { return m_undobuffer.size() != 0; }
    void undo();
    //
    // helpers:
    Point2f pointOffset(const PointMap &pointmap, int side);
    int moveDir(int side);
    //
    void pointPixelBorder(const PointMap &pointmap, std::map<int, int> &relations, SalaShape &shape,
                          int side, PixelRef currpix, PixelRef minpix, bool first);
    // slower point in topmost poly test:
    int pointInPoly(const Point2f &p) const;
    // test if point is inside a particular shape
    bool pointInPoly(const Point2f &p, int shaperef) const;
    // retrieve lists of polys point intersects:
    std::vector<size_t> pointInPolyList(const Point2f &p) const;
    // TODO: Fix casting -1 to size_t
    std::vector<size_t> lineInPolyList(const Line4f &li,
                                       std::optional<size_t> lineref = std::nullopt,
                                       double tolerance = 0.0) const;
    std::vector<size_t> polyInPolyList(int polyref, double tolerance = 0.0) const;
    std::vector<size_t> shapeInPolyList(const SalaShape &shape);
    // helper to make actual test of point in shape:
    std::optional<size_t> testPointInPoly(const Point2f &p, const ShapeRef &shape) const;
    // also allow look for a close polyline:
    int getClosestOpenGeom(const Point2f &p) const;
    // this version simply finds the closest vertex to the point
    Point2f getClosestVertex(const Point2f &p) const;
    // Connect a particular shape into the graph
    size_t connectIntersected(size_t rowid, bool linegraph);
    // Get the connections for a particular line
    std::vector<size_t> getLineConnections(int lineref, double tolerance);
    // Get arbitrary shape connections for a particular shape
    std::vector<size_t> getShapeConnections(int polyref, double tolerance);
    // Make all connections
    void makeShapeConnections();
    //
    const std::vector<Connector> &getConnections() const { return m_connectors; }
    std::vector<Connector> &getConnections() { return m_connectors; }
    //
    bool isAllLineMap() const { return m_mapType == ALLLINEMAP; }
    bool isSegmentMap() const { return m_mapType == SEGMENTMAP; }
    bool isAxialMap() const { return m_mapType == ALLLINEMAP || m_mapType == AXIALMAP; }
    bool isPeshMap() const { return m_mapType == PESHMAP; }
    int getMapType() const { return m_mapType; }
    void setMapType(int newMapType) { m_mapType = newMapType; }
    // Attribute functionality

  public:
    const std::string &getName() const { return m_name; }
    size_t addAttribute(const std::string &name) { return m_attributes->insertOrResetColumn(name); }
    void removeAttribute(size_t col) { m_attributes->removeColumn(col); }
    // I don't want to do this, but every so often you will need to update this table
    // use const version by preference
    AttributeTable &getAttributeTable() { return *m_attributes.get(); }
    const AttributeTable &getAttributeTable() const { return *m_attributes.get(); }
    LayerManagerImpl &getLayers() { return m_layers; }
    const LayerManagerImpl &getLayers() const { return m_layers; }
    AttributeTableHandle &getAttributeTableHandle() { return *m_attribHandle.get(); }
    const AttributeTableHandle &getAttributeTableHandle() const { return *m_attribHandle.get(); }

  public:
    // layer functionality
    bool isLayerVisible(size_t layerid) const { return m_layers.isLayerVisible(layerid); }
    void setLayerVisible(size_t layerid, bool show) { m_layers.setLayerVisible(layerid, show); }

  public:
    double getDisplayMinValue(size_t attributeIdx) const {
        return m_attributes->getColumn(attributeIdx).getStats().min;
    }

    double getDisplayMaxValue(size_t attributeIdx) const {
        return m_attributes->getColumn(attributeIdx).getStats().max;
    }

    double getDefaultMaxValue() const { return m_shapes.size() > 0 ? m_shapes.rbegin()->first : 0; }

    const DisplayParams &getDisplayParams(size_t attributeIdx) const {
        return m_attributes->getColumn(attributeIdx).getDisplayParams();
    }

    // make a local copy of the display params for access speed:
    void setDisplayParams(const DisplayParams &dp, size_t attributeIdx, bool applyToAll = false) {
        if (applyToAll)
            m_attributes->setDisplayParams(dp);
        else
            m_attributes->getColumn(attributeIdx).setDisplayParams(dp);
    }

  public:
    double getDisplayedAverage(size_t attributeIdx) {
        return m_attributes->getColumn(attributeIdx).getStats().total /
               static_cast<double>(m_attributes->getNumRows());
    }

    const std::map<int, SalaShape> getShapesInRegion(const Region4f &r) const;

  public:
    bool hasMapInfoData() const { return m_hasMapInfoData; }
    int loadMifMap(std::istream &miffile, std::istream &midfile);
    bool outputMifMap(std::ostream &miffile, std::ostream &midfile);
    const MapInfoData &getMapInfoData() const { return m_mapinfodata; }

  public:
    // Screen
    std::vector<size_t> makeViewportShapes(const Region4f &viewport) const;
    //
    double getLocationValue(const Point2f &point, std::optional<size_t> attributeIdx) const;

    double getSpacing() const {
        return std::max(m_region.width(), m_region.height()) /
               (10 * log(10.0 + static_cast<double>(m_shapes.size())));
    }
    //
    // dangerous: accessor for the shapes themselves:
    const std::map<int, SalaShape> &getAllShapes() const { return m_shapes; }
    std::map<int, SalaShape> &getAllShapes() { return m_shapes; }
    // required for PixelBase, have to implement your own version of pixelate
    PixelRef pixelate(const Point2f &p, bool constrain = true, int = 1) const override;
    //
  public:
    // file
    bool readNameType(std::istream &stream);
    bool readPart2(std::istream &stream);
    bool readPart3(std::istream &stream);
    virtual std::tuple<bool, bool, bool, int> read(std::istream &stream);

    bool writeNameType(std::ostream &stream) const;
    bool writePart2(std::ostream &stream) const;
    bool writePart3(std::ostream &stream) const;
    virtual bool write(std::ostream &stream,
                       const std::tuple<bool, bool, int> &displayData = std::make_tuple(true, false,
                                                                                        -1)) const;
    //
    bool output(std::ofstream &stream, char delimiter = '\t');
    //

  public:
    bool clearLinks();
    bool linkShapes(const Point2f &p, PixelRef p2);
    bool linkShapesFromRefs(int ref1, int ref2);
    bool linkShapes(size_t index1, size_t index2);
    bool linkShapes(size_t id1, int dir1, size_t id2, int dir2, float weight);
    bool unlinkShapes(const Point2f &p, PixelRef p2);
    bool unlinkShapes(const Point2f &p1, const Point2f &p2);
    bool unlinkShapesFromRefs(int index1, int index2);
    bool unlinkShapes(size_t index1, size_t index2);
    bool unlinkShapesByKey(int key1, int key2);
    bool unlinkShapeSet(std::istream &idset, int refcol);

  public:
    // generic for all types of graphs
    bool findNextLinkLine() const;
    Line4f getNextLinkLine() const;
    std::vector<SimpleLine> getAllLinkLines();

    const std::vector<OrderedSizeTPair> &getLinks() const { return m_links; }
    const std::vector<OrderedSizeTPair> &getUnlinks() const { return m_unlinks; }
    std::vector<Point2f> getAllUnlinkPoints();
    void outputUnlinkPoints(std::ofstream &stream, char delim);

  public:
    std::vector<Line4f> getAllShapesAsLines() const;
    std::vector<SimpleLine> getAllShapesAsSimpleLines() const;
    std::vector<std::pair<SimpleLine, PafColor>>
    getAllSimpleLinesWithColour(const std::set<int> &selSet);
    std::vector<std::pair<std::vector<Point2f>, PafColor>>
    getAllPolygonsWithColour(const std::set<int> &selSet);
    std::vector<std::pair<Point2f, PafColor>> getAllPointsWithColour(const std::set<int> &selSet);
    bool importLines(const std::vector<Line4f> &lines, const depthmapX::Table &data);
    bool importLinesWithRefs(const std::map<int, Line4f> &lines, const depthmapX::Table &data);
    bool importPoints(const std::vector<Point2f> &points, const depthmapX::Table &data);
    bool importPointsWithRefs(const std::map<int, Point2f> &points, const depthmapX::Table &data);
    bool importPolylines(const std::vector<depthmapX::Polyline> &lines,
                         const depthmapX::Table &data);
    bool importPolylinesWithRefs(const std::map<int, depthmapX::Polyline> &lines,
                                 const depthmapX::Table &data);
    void copyMapInfoBaseData(const ShapeMap &sourceMap);

  private:
    bool importData(const depthmapX::Table &data, std::vector<int> shapeRefs);
};
