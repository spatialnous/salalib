// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Interface: the meta graph loads and holds all sorts of arbitrary data...

#include "agents/agentengine.h" // for agent engine interface
#include "connector.h"
#include "fileproperties.h"
#include "importtypedefs.h"
#include "options.h"
#include "pointdata.h"
#include "shapegraph.h"
#include "shapemap.h"
#include "spacepix.h"

#include "genlib/bsptree.h"
#include "genlib/p2dpoly.h"

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

///////////////////////////////////////////////////////////////////////////////////

class Communicator;

// A meta graph is precisely what it says it is

class MetaGraph : public FileProperties {
  private:
    QtRegion m_region; // easier public for now
    std::string m_name;

  public:
    std::string &getName() { return m_name; }
    std::vector<SpacePixelFile> m_drawingFiles;
    const QtRegion &getRegion() { return m_region; }
    void setRegion(Point2f &bottomLeft, Point2f &topRight) {
        m_region.bottom_left = bottomLeft;
        m_region.top_right = topRight;
    }
    bool isShown() const {
        for (size_t i = 0; i < m_drawingFiles.size(); i++)
            if (m_drawingFiles[i].isShown())
                return true;
        return false;
    }

    // TODO: drawing state functions/fields that should be eventually removed
    void makeViewportShapes(const QtRegion &viewport) const;
    bool findNextShape(bool &nextlayer) const;
    const SalaShape &getNextShape() const {
        return m_drawingFiles[static_cast<size_t>(m_current_layer)].getNextShape();
    }
    mutable int m_current_layer;

    enum {
        ADD = 0x0001,
        REPLACE = 0x0002,
        CAT = 0x0010,
        DXF = 0x0020,
        NTF = 0x0040,
        RT1 = 0x0080,
        GML = 0x0100
    };
    enum {
        NONE = 0x0000,
        POINTMAPS = 0x0002,
        LINEDATA = 0x0004,
        ANGULARGRAPH = 0x0010,
        DATAMAPS = 0x0020,
        AXIALLINES = 0x0040,
        SHAPEGRAPHS = 0x0100,
        BUGGY = 0x8000
    };
    enum { NOT_EDITABLE = 0, EDITABLE_OFF = 1, EDITABLE_ON = 2 };

  protected:
    int m_file_version;
    int m_state;

  public:
    // some display options now set at file level
    bool m_showgrid;
    bool m_showtext;
    //
  public:
    std::vector<ShapeMap> m_dataMaps;

    std::vector<std::unique_ptr<ShapeGraph>> m_shapeGraphs;
    std::optional<size_t> m_displayed_shapegraph = std::nullopt;

  public:
    MetaGraph(std::string name = "");
    ~MetaGraph();
    //
    int getVersion() {
        // note, if unsaved, m_file_version is -1
        return m_file_version;
    }

    std::vector<PointMap> &getPointMaps() { return m_pointMaps; }
    bool hasDisplayedPointMap() const { return m_displayed_pointmap.has_value(); }
    PointMap &getDisplayedPointMap() { return m_pointMaps[m_displayed_pointmap.value()]; }
    const PointMap &getDisplayedPointMap() const {
        return m_pointMaps[m_displayed_pointmap.value()];
    }
    void setDisplayedPointMapRef(size_t map) {
        if (m_displayed_pointmap.has_value() && m_displayed_pointmap != map)
            getDisplayedPointMap().clearSel();
        m_displayed_pointmap = map;
    }
    size_t getDisplayedPointMapRef() const { return m_displayed_pointmap.value(); }
    void redoPointMapBlockLines() // (flags blockedlines, but also flags that you need to rebuild a
                                  // bsp tree if you have one)
    {
        for (auto &pointMap : m_pointMaps) {
            pointMap.m_blockedlines = false;
        }
    }
    size_t addNewPointMap(const std::string &name = std::string("VGA Map"));

  private:
    std::vector<PointMap> m_pointMaps;
    std::optional<size_t> m_displayed_pointmap = std::nullopt;

    // helpful to know this for creating fewest line maps, although has to be reread at input
    std::optional<size_t> m_all_line_map = std::nullopt;

    void removePointMap(size_t i) {
        if (m_displayed_pointmap.has_value()) {
            if (m_pointMaps.size() == 1)
                m_displayed_pointmap = std::nullopt;
            else if (m_displayed_pointmap.value() != 0 && m_displayed_pointmap.value() >= i)
                m_displayed_pointmap.value()--;
        }
        m_pointMaps.erase(std::next(m_pointMaps.begin(), static_cast<int>(i)));
    }

    bool readPointMaps(std::istream &stream);
    bool writePointMaps(std::ofstream &stream, bool displayedmaponly = false);

  public:
    int getState() const { return m_state; }
    // use with caution: only very rarely needed outside MetaGraph itself
    void setState(int state) { m_state = state; }

    int loadLineData(Communicator *communicator, int load_type);
    void writeMapShapesAsCat(ShapeMap &map, std::ostream &stream);
    int loadCat(std::istream &stream, Communicator *communicator);
    int loadRT1(const std::vector<std::string> &fileset, Communicator *communicator);
    ShapeMap &createNewShapeMap(depthmapX::ImportType mapType, std::string name);
    void deleteShapeMap(depthmapX::ImportType mapType, ShapeMap &shapeMap);
    void updateParentRegions(ShapeMap &shapeMap);
    bool clearPoints();
    bool setGrid(double spacing, const Point2f &offset = Point2f()); // override of PointMap
    bool makePoints(const Point2f &p, int semifilled,
                    Communicator *communicator = NULL); // override of PointMap
    std::vector<Line> getShownDrawingFilesAsLines();
    std::vector<SalaShape> getShownDrawingFilesAsShapes();
    bool makeGraph(Communicator *communicator, int algorithm, double maxdist);
    bool unmakeGraph(bool removeLinks);
    bool analyseGraph(Communicator *communicator, Options options,
                      bool simple_version); // <- options copied to keep thread safe
    //
    // helpers for editing maps
    bool isEditableMap();
    ShapeMap &getEditableMap();
    // currently only making / moving lines, but should be able to extend this to polys fairly
    // easily:
    bool makeShape(const Line &line);
    bool moveSelShape(const Line &line);
    // onto polys as well:
    int polyBegin(const Line &line);
    bool polyAppend(int shape_ref, const Point2f &point);
    bool polyClose(int shape_ref);
    bool polyCancel(int shape_ref);
    //
    size_t addShapeGraph(std::unique_ptr<ShapeGraph> &shapeGraph);
    size_t addShapeGraph(const std::string &name, int type);
    size_t addShapeMap(const std::string &name);
    void removeDisplayedMap();
    //
    // various map conversions
    bool convertDrawingToAxial(Communicator *comm,
                               std::string layer_name); // n.b., name copied for thread safety
    bool convertDataToAxial(Communicator *comm, std::string layer_name, bool keeporiginal,
                            bool pushvalues);
    bool convertDrawingToSegment(Communicator *comm, std::string layer_name);
    bool convertDataToSegment(Communicator *comm, std::string layer_name, bool keeporiginal,
                              bool pushvalues);
    bool convertToData(Communicator *, std::string layer_name, bool keeporiginal, int shapeMapType,
                       bool copydata);
    bool convertToDrawing(Communicator *, std::string layer_name, bool fromDisplayedDataMap);
    bool convertToConvex(Communicator *comm, std::string layer_name, bool keeporiginal,
                         int shapeMapType, bool copydata);
    bool convertAxialToSegment(Communicator *comm, std::string layer_name, bool keeporiginal,
                               bool pushvalues, double stubremoval);
    int loadMifMap(Communicator *comm, std::istream &miffile, std::istream &midfile);
    bool makeAllLineMap(Communicator *communicator, const Point2f &seed);
    bool makeFewestLineMap(Communicator *communicator, int replace);
    bool analyseAxial(Communicator *communicator, Options options,
                      bool); // <- options copied to keep thread safe
    bool analyseSegmentsTulip(Communicator *communicator,
                              Options options); // <- options copied to keep thread safe
    bool analyseSegmentsAngular(Communicator *communicator,
                                Options options); // <- options copied to keep thread safe
    bool analyseTopoMetMultipleRadii(Communicator *communicator,
                                     Options options); // <- options copied to keep thread safe
    bool analyseTopoMet(Communicator *communicator,
                        Options options); // <- options copied to keep thread safe
    //
    bool hasAllLineMap() { return m_all_line_map.has_value(); }
    bool hasFewestLineMaps() {
        for (const auto &shapeGraph : m_shapeGraphs) {
            if (shapeGraph->getName() == "Fewest-Line Map (Subsets)" ||
                shapeGraph->getName() == "Fewest Line Map (Subsets)" ||
                shapeGraph->getName() == "Fewest-Line Map (Minimal)" ||
                shapeGraph->getName() == "Fewest Line Map (Minimal)") {
                return true;
            }
        }
        return false;
    }
    enum { PUSH_FUNC_MAX = 0, PUSH_FUNC_MIN = 1, PUSH_FUNC_AVG = 2, PUSH_FUNC_TOT = 3 };
    bool pushValuesToLayer(int desttype, size_t destlayer, int push_func, bool count_col = false);
    bool pushValuesToLayer(int sourcetype, size_t sourcelayer, int desttype, size_t destlayer,
                           std::optional<size_t> col_in, size_t col_out, int push_func,
                           bool count_col = false);
    //
    std::optional<size_t> getDisplayedMapRef() const;
    //
    // NB -- returns 0 (not editable), 1 (editable off) or 2 (editable on)
    int isEditable() const;
    bool canUndo() const;
    void undo();

    std::optional<size_t> m_displayed_datamap = std::nullopt;
    bool hasDisplayedDataMap() const { return m_displayed_datamap.has_value(); }
    ShapeMap &getDisplayedDataMap() { return m_dataMaps[m_displayed_datamap.value()]; }
    const ShapeMap &getDisplayedDataMap() const { return m_dataMaps[m_displayed_datamap.value()]; }
    size_t getDisplayedDataMapRef() const { return m_displayed_datamap.value(); }

    void removeDataMap(size_t i) {
        if (m_displayed_datamap.has_value()) {
            if (m_dataMaps.size() == 1)
                m_displayed_datamap = std::nullopt;
            else if (m_displayed_datamap.value() != 0 && m_displayed_datamap.value() >= i)
                m_displayed_datamap.value()--;
        }
        m_dataMaps.erase(std::next(m_dataMaps.begin(), static_cast<int>(i)));
    }

    void setDisplayedDataMapRef(size_t map) {
        if (m_displayed_datamap.has_value() && m_displayed_datamap != map)
            getDisplayedDataMap().clearSel();
        m_displayed_datamap = map;
    }

    template <class T>
    std::optional<size_t> getMapRef(std::vector<T> &maps, const std::string &name) const {
        // note, only finds first map with this name
        for (size_t i = 0; i < maps.size(); i++) {
            if (maps[i].getName() == name)
                return std::optional<size_t>{i};
        }
        return std::nullopt;
    }

    std::vector<std::unique_ptr<ShapeGraph>> &getShapeGraphs() { return m_shapeGraphs; }
    bool hasDisplayedShapeGraph() const { return m_displayed_shapegraph.has_value(); }
    ShapeGraph &getDisplayedShapeGraph() {
        return *m_shapeGraphs[m_displayed_shapegraph.value()].get();
    }
    const ShapeGraph &getDisplayedShapeGraph() const {
        return *m_shapeGraphs[m_displayed_shapegraph.value()].get();
    }
    void setDisplayedShapeGraphRef(size_t map) {
        if (m_displayed_shapegraph.has_value() && m_displayed_shapegraph != map)
            getDisplayedShapeGraph().clearSel();
        m_displayed_shapegraph = map;
    }
    size_t getDisplayedShapeGraphRef() const { return m_displayed_shapegraph.value(); }

    void removeShapeGraph(size_t i) {
        if (m_displayed_shapegraph.has_value()) {
            if (m_shapeGraphs.size() == 1)
                m_displayed_shapegraph = std::nullopt;
            else if (m_displayed_shapegraph.value() != 0 && m_displayed_shapegraph.value() >= i)
                m_displayed_shapegraph.value()--;
        }
        m_shapeGraphs.erase(std::next(m_shapeGraphs.begin(), static_cast<int>(i)));
    }

    bool readShapeGraphs(std::istream &stream);
    bool writeShapeGraphs(std::ofstream &stream, bool displayedmaponly = false);

    std::vector<ShapeMap> &getDataMaps() { return m_dataMaps; }

    bool readDataMaps(std::istream &stream);
    bool writeDataMaps(std::ofstream &stream, bool displayedmaponly = false);

    //
    int getDisplayedMapType();
    AttributeTable &getDisplayedMapAttributes();
    bool hasVisibleDrawingLayers();
    QtRegion getBoundingBox() const;
    //
    int getDisplayedAttribute() const;
    void setDisplayedAttribute(int col);
    std::optional<size_t> addAttribute(const std::string &name);
    void removeAttribute(int col);
    bool isAttributeLocked(size_t col);
    AttributeTable &getAttributeTable(std::optional<size_t> type = std::nullopt,
                                      std::optional<size_t> layer = std::nullopt);
    const AttributeTable &getAttributeTable(std::optional<size_t> type = std::nullopt,
                                            std::optional<size_t> layer = std::nullopt) const;

    int getLineFileCount() const { return (int)m_drawingFiles.size(); }

    // Quick mod - TV
    const std::string &getLineFileName(size_t fileIdx) const {
        return m_drawingFiles[fileIdx].getName();
    }
    size_t getLineLayerCount(size_t fileIdx) const {
        return m_drawingFiles[fileIdx].m_spacePixels.size();
    }

    ShapeMap &getLineLayer(size_t fileIdx, size_t layerIdx) {
        return m_drawingFiles[fileIdx].m_spacePixels[layerIdx];
    }
    const ShapeMap &getLineLayer(size_t fileIdx, size_t layerIdx) const {
        return m_drawingFiles[fileIdx].m_spacePixels[layerIdx];
    }
    //
    // Some error handling -- the idea is that you catch the error in MetaGraph,
    // return a generic error code and then get your front end to interrogate the
    // last error (pretty much as per standard Unix etc).
    // May have problems with multithreading.
  public:
    class Error {
      protected:
        std::string error;

      public:
        Error(const std::string &err = std::string()) { error = err; }
    };

  protected:
    Error m_last_error;

  public:
    Error &getLastError() { return m_last_error; }
    // for drawing either axial analysis or VGA
  protected:
    int m_view_class;

  public:
    enum {
        SHOWHIDEVGA = 0x0100,
        SHOWVGATOP = 0x0200,
        SHOWHIDEAXIAL = 0x0400,
        SHOWAXIALTOP = 0x0800,
        SHOWHIDESHAPE = 0x1000,
        SHOWSHAPETOP = 0x2000
    };
    enum {
        VIEWNONE = 0x00,
        VIEWVGA = 0x01,
        VIEWBACKVGA = 0x02,
        VIEWAXIAL = 0x04,
        VIEWBACKAXIAL = 0x08,
        VIEWDATA = 0x20,
        VIEWBACKDATA = 0x40,
        VIEWFRONT = 0x25
    };
    //
    int getViewClass() { return m_view_class; }
    // These functions make specifying conditions to do things much easier:
    bool viewingNone() { return (m_view_class == VIEWNONE); }
    bool viewingProcessed() {
        return ((m_view_class & (VIEWAXIAL | VIEWDATA)) ||
                (m_view_class & VIEWVGA && getDisplayedPointMap().isProcessed()));
    }
    bool viewingShapes() { return (m_view_class & (VIEWAXIAL | VIEWDATA)) != 0; }
    bool viewingProcessedLines() { return ((m_view_class & VIEWAXIAL) == VIEWAXIAL); }
    bool viewingProcessedShapes() { return ((m_view_class & VIEWDATA) == VIEWDATA); }
    bool viewingProcessedPoints() {
        return ((m_view_class & VIEWVGA) && getDisplayedPointMap().isProcessed());
    }
    bool viewingUnprocessedPoints() {
        return ((m_view_class & VIEWVGA) && !getDisplayedPointMap().isProcessed());
    }
    //
    bool setViewClass(int command);
    //
    double getLocationValue(const Point2f &point);
    //
  public:
    // these are dependent on what the view class is:
    bool isSelected() // does a selection exist
    {
        if (m_view_class & VIEWVGA)
            return getDisplayedPointMap().isSelected();
        else if (m_view_class & VIEWAXIAL)
            return getDisplayedShapeGraph().hasSelectedElements();
        else if (m_view_class & VIEWDATA)
            return getDisplayedDataMap().hasSelectedElements();
        else
            return false;
    }
    bool setCurSel(QtRegion &r, bool add = false) // set current selection
    {
        if (m_view_class & VIEWAXIAL)
            return getDisplayedShapeGraph().setCurSel(r, add);
        else if (m_view_class & VIEWDATA)
            return getDisplayedDataMap().setCurSel(r, add);
        else if (m_view_class & VIEWVGA)
            return getDisplayedPointMap().setCurSel(r, add);
        else if (m_state & POINTMAPS &&
                 !getDisplayedPointMap().isProcessed()) // this is a default select application
            return getDisplayedPointMap().setCurSel(r, add);
        else if (m_state & DATAMAPS) // I'm not sure why this is a possibility, but it appears
                                     // you might have state & DATAMAPS without VIEWDATA...
            return getDisplayedDataMap().setCurSel(r, add);
        else
            return false;
    }
    bool clearSel() {
        // really needs a separate clearSel for the datalayers... at the moment this is handled
        // in PointMap
        if (m_view_class & VIEWVGA)
            return getDisplayedPointMap().clearSel();
        else if (m_view_class & VIEWAXIAL)
            return getDisplayedShapeGraph().clearSel();
        else if (m_view_class & VIEWDATA)
            return getDisplayedDataMap().clearSel();
        else
            return false;
    }
    int getSelCount() {
        if (m_view_class & VIEWVGA)
            return getDisplayedPointMap().getSelCount();
        else if (m_view_class & VIEWAXIAL)
            return (int)getDisplayedShapeGraph().getSelCount();
        else if (m_view_class & VIEWDATA)
            return (int)getDisplayedDataMap().getSelCount();
        else
            return 0;
    }
    float getSelAvg() {
        if (m_view_class & VIEWVGA)
            return (float)getDisplayedPointMap().getDisplayedSelectedAvg();
        else if (m_view_class & VIEWAXIAL)
            return (float)getDisplayedShapeGraph().getDisplayedSelectedAvg();
        else if (m_view_class & VIEWDATA)
            return (float)getDisplayedDataMap().getDisplayedSelectedAvg();
        else
            return -1.0f;
    }
    QtRegion getSelBounds() {
        if (m_view_class & VIEWVGA)
            return getDisplayedPointMap().getSelBounds();
        else if (m_view_class & VIEWAXIAL)
            return getDisplayedShapeGraph().getSelBounds();
        else if (m_view_class & VIEWDATA)
            return getDisplayedDataMap().getSelBounds();
        else
            return QtRegion();
    }
    // setSelSet expects a set of ref ids:
    void setSelSet(const std::vector<int> &selset, bool add = false) {
        if (m_view_class & VIEWVGA && m_state & POINTMAPS)
            getDisplayedPointMap().setCurSel(selset, add);
        else if (m_view_class & VIEWAXIAL)
            getDisplayedShapeGraph().setCurSel(selset, add);
        else // if (m_view_class & VIEWDATA)
            getDisplayedDataMap().setCurSel(selset, add);
    }
    std::set<int> &getSelSet() {
        if (m_view_class & VIEWVGA && m_state & POINTMAPS)
            return getDisplayedPointMap().getSelSet();
        else if (m_view_class & VIEWAXIAL)
            return getDisplayedShapeGraph().getSelSet();
        else // if (m_view_class & VIEWDATA)
            return getDisplayedDataMap().getSelSet();
    }
    const std::set<int> &getSelSet() const {
        if (m_view_class & VIEWVGA && m_state & POINTMAPS)
            return getDisplayedPointMap().getSelSet();
        else if (m_view_class & VIEWAXIAL)
            return getDisplayedShapeGraph().getSelSet();
        else // if (m_view_class & VIEWDATA)
            return getDisplayedDataMap().getSelSet();
    }
    //
  public:
    // no longer supported
    // int addLineDynamic(const Line& l);
    // bool removeLineDynamic(LineKey linekey);
    //
    // Agent engine interface:
  protected:
    AgentEngine m_agent_engine;

  public:
    AgentEngine &getAgentEngine() { return m_agent_engine; }
    void runAgentEngine(Communicator *comm);
    //
  public:
    // thru vision
    bool analyseThruVision(Communicator *comm = NULL,
                           std::optional<size_t> gatelayer = std::nullopt);
    // BSP tree for making isovists
  protected:
    BSPNode *m_bsp_root;
    bool m_bsp_tree;

  public:
    bool makeBSPtree(Communicator *communicator = NULL);
    void resetBSPtree() { m_bsp_tree = false; }
    // returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
    int makeIsovist(Communicator *communicator, const Point2f &p, double startangle = 0,
                    double endangle = 0, bool simple_version = true);
    std::set<std::string> setIsovistData(Isovist &isovist, AttributeTable &table, AttributeRow &row,
                                         bool simple_version);
    // returns 0: fail, 1: made isovist, 2: made isovist and added new shapemap layer
    int makeIsovistPath(Communicator *communicator, double fov_angle = 2.0 * M_PI,
                        bool simple_version = true);
    bool makeIsovist(const Point2f &p, Isovist &iso);

  protected:
    // properties
  public:
    // a few read-write returns:
    enum {
        OK,
        WARN_BUGGY_VERSION,
        WARN_CONVERTED,
        NOT_A_GRAPH,
        DAMAGED_FILE,
        DISK_ERROR,
        NEWER_VERSION,
        DEPRECATED_VERSION
    };
    // likely to use communicator if too slow...
    int readFromFile(const std::string &filename);
    int readFromStream(std::istream &stream, const std::string &);
    int write(const std::string &filename, int version, bool currentlayer = false);
    //
    std::vector<SimpleLine> getVisibleDrawingLines();

  protected:
    std::streampos skipVirtualMem(std::istream &stream);
};
