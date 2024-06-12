// SPDX-FileCopyrightText: 2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is a hex pattern for reading graph files in imhex

#include "std/mem.pat"

#pragma pattern_limit 2042072
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
    u64 key;
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
    u64 att_availableLayers;
    u64 att_visibleLayers;
    u32 att_layerCount;
    AttributeLayer layers[att_layerCount];
    u32 att_colCount;
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
    s16 num_vec;
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
struct Node {
    Bin bins[32];
    PixelRef occ_bins[32];
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
    u32 rows;
    u32 columns;
    u32 pointCount;
    Point2f bottom_left;
    u32 displayed_attribute;
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

enum MapType : char {
    LineMap = 'l',
    ShapeGraphs = 'x',
    PointMaps = 'p'
};

struct MapSelector {
    MapType type;
    
    if (type == MapType::LineMap) {
        LineData ld;
    } else if (type == MapType::ShapeGraphs) {
        ShapeGraphs shapeGraphs;
    } else if (type == MapType::PointMaps) {
        PointMaps pointMaps;
    }
};

struct LineDataOption {
    try {
        char marker;
        if (marker == 'x')
            continue;
        LineData ld;
    }
};

// file structs
struct header {
    char grf[0x03];
    u32 version;
    char unknown_structure[0x13];
    FBLString stringDate;
    FBLString stringVersion;
    padding[0xC];
};

struct File {
    header hdr;
    MapSelector m;
    MapSelector p;
    //MapSelector m[while(!std::mem::eof())];
};

File f @0x00;
