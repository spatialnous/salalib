// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is a hex pattern for reading graph files in imhex

#include "std/mem.pat"

#pragma pattern_limit 2942072
#pragma array_limit 65536

struct FBLString {
    u32 length;
    char string[length];
};
struct Point2f {
    double x, y;
};
struct QtRegion {
    double minX;
    double minY;
    double maxX;
    double maxY;
};
struct Line : QtRegion {
    u8 parity;
    u8 dir;
    padding[6];
};
enum ShapeType : u8 {
    Point = 0x1,
    Line = 0x2,
    Polygon = 0x4,
    Circle = 0x8,
    ClosedPoly = 0x44,
    Circle = 0xC4
};
struct SalaShape {
    u32 key;
    ShapeType type;
    Line region;
    Point2f centroid;
    double area;
    double perimeter;
    s32 point_count;
    Point2f points[point_count];
};
struct SalaObject {
    u32 key;
    Point2f centroid;
};
struct AttributeLayer {
    s64 key;
    FBLString name;
};
struct DisplayParameters {
    float dp_blue, dp_red;
    u32 dp_colorscale;
};
struct AttributeColumn {
    FBLString name;
    float min, max;
    double total;
    u32 physical_col;
    bool hidden;
    bool locked;
    DisplayParameters col_DisplayParams;
    FBLString att_colFormula;
};
struct FloatVector {
    u32 length;
    float values[length];
};
struct IntVector {
    u32 length;
    u32 values[length];
};
struct AttributeRow {
    u32 key;
    u64 num_layers;
    FloatVector att_rowValues;
};
struct AttributeTable {
    s64 att_availableLayers;
    s64 att_visibleLayers;
    s32 att_layerCount;
    AttributeLayer layers[att_layerCount];
    s32 att_colCount;
    AttributeColumn columns[att_colCount];
    u32 att_rowCount;
    AttributeRow rows[att_rowCount];
    DisplayParameters att_DisplayParams;
};
struct SegmentRef {
    u8 dir;
    padding[3];
    u32 key;
};
struct SegmentRefValue {
    SegmentRef segmentref;
    float value;
};
struct SegmentRefFloatMap {
    u32 length;
    SegmentRefValue values[length];
};
struct Connector {
    IntVector connections;
    u32 conn_segmentAxialRef;
    SegmentRefFloatMap forward_segconns;
    SegmentRefFloatMap back_segconns;
};
struct OrderedIntPair {
    u32 x, y;
};
struct IntPairVector {
    u32 length;
    OrderedIntPair values[length];
};
struct MapInfoData {
    FBLString mid_version;
    FBLString mid_charset;
    char delimeter;
    FBLString mid_index;
    MapInfoData mid_coordsys;
    FBLString mid_bounds;
};
enum Marker: char {
    MapInfo = 'm',
    End = 'x'
};

struct MarkerSelector {
    Marker marker;
    
    if (marker == Marker::MapInfo) {
        MapInfoData mapInfoData;
    }
    else if (marker == Marker::End) {
       // end_marker
    }
};
struct ShapeMap {
    FBLString map_name;
    u32 map_type;
    bool show_map;
    bool is_editable;
    QtRegion region;
    u32 rows;
    u32 cols;
    u32 object_reference;
    u32 shape_reference;
    u32 shape_count;
    SalaShape shapes[shape_count];
    u32 object_count;
    SalaObject objects[object_count];
    AttributeTable attributes;
    u32 dm_displayedAttribute;
    u32 dm_numConnectors;
    Connector connectors[dm_numConnectors];
    IntPairVector dm_links;
    IntPairVector dm_unlinks;
    MarkerSelector markerSelectors[];
};
struct SpacePixel {
    FBLString planFileName;
    QtRegion sp_Region;
    u32 sp_numShapeMaps;
    ShapeMap sp_layerShapeMap[sp_numShapeMaps];
};
struct LineData {
    FBLString SuperSpacePixelName;
    QtRegion region;
    s32 num_space_pixels;
    SpacePixel space_pixels[num_space_pixels];
};
struct ShapeGraph {
    u32 sg_keyVertexCount;
    u32 sg_numKeyVertices;
    IntVector keyVertices[sg_numKeyVertices];
    ShapeMap sg_shapeMap;
};
struct AxialVertexKey {
    u32 avk_refKey;
    u16 avk_refA;
    u16 avk_refB;
};
struct RadialKey {
    AxialVertexKey rk_axialVertexKey;
    float rk_ang;
    u8 rk_segend;
    padding[3];
};
struct RadialLine {
    RadialKey sg_radialKey;
    Point2f rl_openSpace;
    Point2f rl_keyVertex;
    Point2f rl_nextVertex;
};
struct PolyConnector {
    Line pc_line;
    RadialKey pc_radialKey;
};
struct ShapeGraphs {
    u32 displayed_map;
    u32 sg_numShapeMaps;
    ShapeGraph shapeGraphs[sg_numShapeMaps];
    u32 sg_numPolyConnectors;
    PolyConnector sg_polyConnectors[sg_numPolyConnectors];
    u32 sg_numRadialLines;
    RadialLine sg_radialLine[sg_numRadialLines];
};

struct PixelRef {
    u16 x, y;
};
struct PixelVec {
    PixelRef start;
    u16 length;
};
bitfield ShiftLength {
    unsigned shift : 4;
    unsigned runlength : 12;
}; 
struct PixelVecWithContext {
    u16 primary;
    ShiftLength shiftLength;
};
struct NodeBinMultiPixVec {
    u16 num_vec;
    PixelVec pixVec;
    PixelVecWithContext pixVecC[num_vec - 1];
};
struct Bin {
    s8 dir;
    u16 node_count;
    u32 distance;
    u32 occ_distance;
    if (node_count >0) {
        if (dir & 0x0c) {
            PixelVec pixelVec;
        } else {
            NodeBinMultiPixVec multiPixelVec;
        }
    }
};
struct PixelRefVec {
    u32 npixrefs;
    PixelRef refs[npixrefs];
};
struct Node {
    Bin bins[32];
    PixelRefVec occ_bins[32];
};
struct SalaPoint {
    u32 state;
    u32 block;
    u32 misc;
    char grid_connections;
    PixelRef merge;
    bool have_node;
    if (have_node) {
        Node node;
    }
    Point2f location;
};
struct PointMap {
    FBLString name;
    double spacing;
    s32 rows;
    s32 columns;
    u32 filledPointCount;
    Point2f bottom_left;
    s32 displayed_attribute;
    AttributeTable attributes;
    SalaPoint points[rows * columns];
    bool processed;
    bool boundary_graph;
};
struct PointMaps {
    u32 displayed_map;
    u32 count;
    PointMap maps[count];
};

struct DataMaps {
    u32 displayed_map;
    u32 count;
    ShapeMap maps[count];
};

enum MapType : char {
    LineData = 'l',
    ShapeGraphs = 'x',
    PointMaps = 'p',
    DataMaps = 's'
};

struct LineDataOption {
    try {
        char marker;
        if (marker != MapType::LineData)
            continue;
        LineData lineData;
    }
};

struct PointMapsOption {
    try {
        char marker;
        if (marker != MapType::PointMaps)
            continue;
        PointMaps pointMaps;
    }
};
struct ShapeGraphsOption {
    try {
        char marker;
        if (marker != MapType::ShapeGraphs)
            continue;
        ShapeGraphs shapeGraphs;
    }
};
struct DataMapsOption {
    try {
        char marker;
        if (marker != MapType::DataMaps)
            continue;
        DataMaps dataMaps;
    }
};

// file structs
struct header {
    char grf[0x03];
    u32 version;
    s32 state;
    s32 viewClass;
    bool showGrid;
    bool showText;
    char xmarker;
    FBLString creator;
    FBLString organisation;
    FBLString date;
    FBLString program;
    FBLString title;
    FBLString location;
    FBLString description;
};

struct File {
    header hdr;
    LineDataOption ld;
    PointMapsOption pm;
    ShapeGraphsOption sg;
    DataMapsOption dm;
};

File f @0x00;
