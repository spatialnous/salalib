// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

// This is my code to make a set of axial lines from a set of boundary lines

#include "shapemap.hpp"

#include "attributetable.hpp"
#include "attributetablehelpers.hpp"
#include "parsers/mapinfodata.hpp" // for mapinfo interface
#include "pointmap.hpp"
#include "tolerances.hpp"

#include "genlib/containerutils.hpp"
#include "genlib/exceptions.hpp"
#include "genlib/readwritehelpers.hpp"
#include "genlib/stringutils.hpp"

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <time.h>
#include <unordered_map>
#include <unordered_set>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// the replacement for datalayers

ShapeMap::ShapeMap(const std::string &name, int type)
    : AttributeMap(name, std::unique_ptr<AttributeTable>(new AttributeTable())), m_mapType(type),
      m_objRef(-1), m_pixelShapes(0, 0), m_shapes(), m_undobuffer(), m_connectors(),
      m_tolerance(0.0), m_links(), m_unlinks(), m_mapinfodata(), m_hasMapInfoData(false),
      m_hasgraph(false), _padding0(0), _padding1(0) {

    // shape and object counters

    // data (MUST be set before use)

    //
}

//////////////////////////////////////////////////////////////////////////////////////////

// this can be reinit as well

void ShapeMap::init(size_t size, const Region4f &r) {
    m_rows = static_cast<size_t>(
        std::min(std::max(20, static_cast<int>(sqrt(static_cast<double>(size)))), 32768));
    m_cols = static_cast<size_t>(
        std::min(std::max(20, static_cast<int>(sqrt(static_cast<double>(size)))), 32768));
    if (m_region.atZero()) {
        m_region = r;
    } else {
        m_region = m_region.runion(r);
    }
    // calculate geom data:
    m_tolerance = std::max(m_region.width(), m_region.height()) * TOLERANCE_A;
    //
    m_pixelShapes = genlib::ColumnMatrix<std::vector<ShapeRef>>(m_rows, m_cols);
}

// this makes an exact copy, keep the reference numbers and so on:

void ShapeMap::copy(const ShapeMap &sourcemap, int copyflags, bool copyMapType) {
    if (copyMapType) {
        m_mapType = sourcemap.m_mapType;
    }
    if ((copyflags & ShapeMap::COPY_GEOMETRY) == ShapeMap::COPY_GEOMETRY) {
        m_shapes.clear();
        init(sourcemap.m_shapes.size(), sourcemap.m_region);
        for (const auto &shape : sourcemap.m_shapes) {
            // using makeShape is actually easier than thinking about a total copy:
            makeShape(shape.second, shape.first);
            // note that addShape automatically adds the attribute row
        }
    }

    if ((copyflags & ShapeMap::COPY_ATTRIBUTES) == ShapeMap::COPY_ATTRIBUTES) {
        // assumes attribute rows are filled in already

        // TODO: Compatibility. The columns are sorted in the old implementation so
        // they are also passed sorted in the conversion:

        std::vector<size_t> indices(sourcemap.m_attributes->getNumColumns());
        std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return sourcemap.m_attributes->getColumnName(a) <
                   sourcemap.m_attributes->getColumnName(b);
        });

        for (auto idx : indices) {
            auto outcol =
                m_attributes->insertOrResetColumn(sourcemap.m_attributes->getColumnName(idx));
            // n.b. outcol not necessarily the same as incol, although row position in
            // table (j) should match

            auto targetIter = m_attributes->begin();
            for (auto sourceIter = sourcemap.m_attributes->begin();
                 sourceIter != sourcemap.m_attributes->end(); sourceIter++) {
                targetIter->getRow().setValue(outcol, sourceIter->getRow().getValue(idx));
                targetIter++;
            }
        }
    }

    if ((copyflags & ShapeMap::COPY_GRAPH) == ShapeMap::COPY_GRAPH) {
        if (sourcemap.m_hasgraph) {
            m_hasgraph = true;
            // straight copy:
            m_connectors = sourcemap.m_connectors;
            m_links = sourcemap.m_links;
            m_unlinks = sourcemap.m_unlinks;
        }
    }

    // copies mapinfodata (projection data) regardless of copy flags
    if (sourcemap.hasMapInfoData()) {
        m_mapinfodata = MapInfoData();
        m_mapinfodata.m_coordsys = sourcemap.getMapInfoData().m_coordsys;
        m_mapinfodata.m_bounds = sourcemap.getMapInfoData().m_bounds;
        m_hasMapInfoData = true;
    }
}

// Zaps all memory structures, apart from mapinfodata
void ShapeMap::clearAll() {
    m_shapes.clear();
    m_undobuffer.clear();
    m_connectors.clear();
    m_attributes->clear();
    m_links.clear();
    m_unlinks.clear();
    m_region = Region4f();

    m_objRef = -1;
}

///////////////////////////////////////////////////////////////////////////////////////////

int ShapeMap::makePointShapeWithRef(const Point2f &point, int shapeRef, bool tempshape,
                                    const std::map<size_t, float> &extraAttributes) {
    bool boundsGood = true;

    if (!m_region.contains_touch(point)) {
        boundsGood = false;
        init(m_shapes.size(), Region4f(point, point));
    }

    m_shapes.insert(std::make_pair(shapeRef, SalaShape(point)));

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(shapeRef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    if (!tempshape) {
        auto &row = m_attributes->addRow(AttributeKey(shapeRef));
        for (auto &attr : extraAttributes) {
            row.setValue(static_cast<size_t>(attr.first), attr.second);
        }
    }

    return shapeRef;
}

int ShapeMap::makePointShape(const Point2f &point, bool tempshape,
                             const std::map<size_t, float> &extraAttributes) {
    return makePointShapeWithRef(point, getNextShapeKey(), tempshape, extraAttributes);
}

int ShapeMap::makeLineShapeWithRef(const Line4f &line, int shapeRef, bool throughUi, bool tempshape,
                                   const std::map<size_t, float> &extraAttributes) {

    bool boundsGood = true;

    if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
        boundsGood = false;
        init(m_shapes.size(), line);
    }

    // note, shape constructor sets centroid, length etc
    m_shapes.insert(std::make_pair(shapeRef, SalaShape(line)));

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(shapeRef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    if (!tempshape) {
        auto &row = m_attributes->addRow(AttributeKey(shapeRef));
        for (auto &attr : extraAttributes) {
            row.setValue(attr.first, attr.second);
        }
    }

    if (throughUi) {
        // manually add connections:
        if (m_hasgraph) {
            auto rowid = genlib::findIndexFromKey(m_shapes, shapeRef);
            if (rowid < 0) {
                throw new genlib::RuntimeException("Shape reference " + std::to_string(shapeRef) +
                                                   " not found to make line");
            }
            if (isAxialMap()) {
                connectIntersected(
                    static_cast<size_t>(rowid),
                    true); // "true" means line-line intersections only will be applied
            } else {
                connectIntersected(static_cast<size_t>(rowid), false);
            }
        }
        // if through ui, set undo counter:
        m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_CREATED, shapeRef));
    }

    return shapeRef;
}

int ShapeMap::getNextShapeKey() {
    if (m_shapes.size() == 0)
        return 0;
    return m_shapes.rbegin()->first + 1;
}

int ShapeMap::makeLineShape(const Line4f &line, bool throughUi, bool tempshape,
                            const std::map<size_t, float> &extraAttributes) {
    return makeLineShapeWithRef(line, getNextShapeKey(), throughUi, tempshape, extraAttributes);
}

int ShapeMap::makePolyShapeWithRef(const std::vector<Point2f> &points, bool open, int shapeRef,
                                   bool tempshape, const std::map<size_t, float> &extraAttributes) {
    bool boundsGood = true;

    switch (points.size()) {
    case 0:
        return -1;
    case 1:
        return makePointShapeWithRef(points[0], shapeRef, tempshape);
    case 2:
        return makeLineShapeWithRef(Line4f(points[0], points[1]), shapeRef, false,
                                    tempshape); // false is not through ui: there really should be a
                                                // through ui here?
    }

    Region4f region(points[0], points[0]);
    size_t i;
    for (i = 1; i < points.size(); i++) {
        region.encompass(points[i]);
    }
    if (!m_region.contains_touch(region.bottomLeft) || !m_region.contains_touch(region.topRight)) {
        boundsGood = false;
        init(m_shapes.size(), region);
    }

    size_t len = points.size();
    // NOTE: This is commented out deliberately
    // Sometimes you really do want a polyline that forms a loop
    /*
    if (points.head() == points.tail()) {
       len--;
       open = false;
    }
    */

    // not sure if it matters if the polygon is clockwise or anticlockwise...
    // we'll soon tell!

    if (open) {
        m_shapes.insert(std::make_pair(shapeRef, SalaShape(SalaShape::SHAPE_POLY)));
    } else {
        m_shapes.insert(
            std::make_pair(shapeRef, SalaShape(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED)));
    }
    for (i = 0; i < len; i++) {
        m_shapes.rbegin()->second.points.push_back(points[i]);
    }

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(shapeRef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    if (!tempshape) {
        // set centroid now also adds a few other things: as well as area, perimeter
        m_shapes.rbegin()->second.setCentroidAreaPerim();

        auto &row = m_attributes->addRow(AttributeKey(shapeRef));
        for (const auto &attr : extraAttributes) {
            row.setValue(attr.first, attr.second);
        }
    }

    return shapeRef;
}

int ShapeMap::makePolyShape(const std::vector<Point2f> &points, bool open, bool tempshape,
                            const std::map<size_t, float> &extraAttributes) {
    return makePolyShapeWithRef(points, open, getNextShapeKey(), tempshape, extraAttributes);
}

int ShapeMap::makeShape(const SalaShape &poly, int overrideShapeRef,
                        const std::map<size_t, float> &extraAttributes) {
    // overridden shape cannot exist:
    if (overrideShapeRef != -1 && m_shapes.find(overrideShapeRef) != m_shapes.end()) {
        return -1; // failure!
    }

    bool boundsGood = true;

    if (!m_region.contains_touch(poly.m_region.bottomLeft) ||
        !m_region.contains_touch(poly.m_region.topRight)) {
        boundsGood = false;
        init(m_shapes.size(), poly.m_region);
    }

    int shapeRef;
    if (overrideShapeRef == -1) {
        shapeRef = getNextShapeKey();
    } else {
        shapeRef = overrideShapeRef;
    }

    m_shapes.insert(std::make_pair(shapeRef, poly));

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(shapeRef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    auto &row = m_attributes->addRow(AttributeKey(shapeRef));
    for (auto &attr : extraAttributes) {
        row.setValue(attr.first, attr.second);
    }

    return shapeRef;
}

int ShapeMap::makeShapeFromPointSet(const PointMap &pointmap, const std::set<int> &selSet) {
    bool boundsGood = true;
    PixelRefVector selset;
    Point2f offset = Point2f(pointmap.getSpacing() / 2, pointmap.getSpacing() / 2);
    for (auto &sel : selSet) {
        selset.push_back(sel);
        if (!m_region.contains_touch(pointmap.depixelate(sel) - offset) ||
            !m_region.contains_touch(pointmap.depixelate(sel) + offset)) {
            boundsGood = false;
        }
    }
    if (!boundsGood) {
        Region4f r(pointmap.getRegion().bottomLeft - offset,
                   pointmap.getRegion().topRight + offset);
        init(m_shapes.size(), r);
    }
    std::map<int, int> relations;
    for (size_t j = 0; j < selset.size(); j++) {
        PixelRef pix = selset[j];
        auto relation = relations.insert(std::make_pair(pix, ShapeRef::SHAPE_EDGE));
        if (pointmap.includes(pix.right()) && selSet.find(pix.right()) != selSet.end()) {
            relation.first->second &= ~ShapeRef::SHAPE_R;
        }
        if (pointmap.includes(pix.up()) && selSet.find(pix.up()) != selSet.end()) {
            relation.first->second &= ~ShapeRef::SHAPE_T;
        }
        if (pointmap.includes(pix.down()) && selSet.find(pix.down()) != selSet.end()) {
            relation.first->second &= ~ShapeRef::SHAPE_B;
        }
        if (pointmap.includes(pix.left()) && selSet.find(pix.left()) != selSet.end()) {
            relation.first->second &= ~ShapeRef::SHAPE_L;
        }
    }
    // now find pixel with SHAPE_B | SHAPE_L
    PixelRef minpix = NoPixel;

    for (auto &relation : relations) {
        if ((relation.second & (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) ==
            (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) {
            if ((minpix == NoPixel) || (relation.first < static_cast<int>(minpix))) {
                minpix = relation.first;
            }
        }
    }
    // now follow round anticlockwise...
    SalaShape poly(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED);
    pointPixelBorder(pointmap, relations, poly, ShapeRef::SHAPE_L, minpix, minpix, true);

    for (auto relation : relations) {
        if (relation.second != 0) {
            // more than one shape!
            return -1;
        }
    }
    poly.setCentroidAreaPerim();

    int newShapeRef = getNextShapeKey();
    m_shapes.insert(std::make_pair(newShapeRef, poly));

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(newShapeRef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    m_attributes->addRow(AttributeKey(newShapeRef));

    return newShapeRef;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::convertPointsToPolys(
    double polyRadius, std::optional<std::reference_wrapper<const std::set<int>>> selSet) {
    // I'm not sure quite how easy this will be...
    Region4f region;

    bool doneSomething = false;

    // replace the points with polys
    for (auto shape : m_shapes) {
        if (selSet.has_value() && selSet->get().find(shape.first) == selSet->get().end()) {
            continue;
        }
        if (shape.second.isPoint()) {
            doneSomething = true;
            // remove old spatial index
            removePolyPixels(shape.first);
            // construct a poly from the point:
            Point2f p = shape.second.getCentroid();
            //
            if (region.atZero()) {
                region = Region4f(p, p);
            }
            // replace with a polygon:
            shape.second = SalaShape(SalaShape::SHAPE_POLY | SalaShape::SHAPE_CLOSED);
            for (int k = 0; k < 8; k++) {
                Point2f polyP;
                if (k == 0) {
                    polyP.x = p.x + polyRadius;
                    polyP.y = p.y;
                } else if (k == 1) {
                    polyP.x = p.x + polyRadius * pafmath_M_ROOT_1_2;
                    polyP.y = p.y + polyRadius * pafmath_M_ROOT_1_2;
                } else if (k == 2) {
                    polyP.x = p.x;
                    polyP.y = p.y + polyRadius;
                } else if (k == 3) {
                    polyP.x = p.x - polyRadius * pafmath_M_ROOT_1_2;
                    polyP.y = p.y + polyRadius * pafmath_M_ROOT_1_2;
                } else if (k == 4) {
                    polyP.x = p.x - polyRadius;
                    polyP.y = p.y;
                } else if (k == 5) {
                    polyP.x = p.x - polyRadius * pafmath_M_ROOT_1_2;
                    polyP.y = p.y - polyRadius * pafmath_M_ROOT_1_2;
                } else if (k == 6) {
                    polyP.x = p.x;
                    polyP.y = p.y - polyRadius;
                } else if (k == 7) {
                    polyP.x = p.x + polyRadius * pafmath_M_ROOT_1_2;
                    polyP.y = p.y - polyRadius * pafmath_M_ROOT_1_2;
                }
                region.encompass(polyP);
                shape.second.points.push_back(polyP);
            }
            shape.second.setCentroidAreaPerim();
        }
    }

    if (doneSomething) {
        // spatially reindex (simplest just to redo everything)
        init(m_shapes.size(), region);

        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::moveShape(int shaperef, const Line4f &line, bool undoing) {
    bool boundsGood = true;

    auto shapeIter = m_shapes.find(shaperef);
    if (shapeIter == m_shapes.end()) {
        return false;
    }

    // remove shape from the pixel grid
    removePolyPixels(shaperef); // done first, as all interface references use this list

    if (!undoing) {
        // set undo counter, but only if this is not an undo itself:
        m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_MOVED, shaperef));
        m_undobuffer.back().geometry = shapeIter->second;
    }

    if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
        boundsGood = false;
        init(m_shapes.size(), line);
    }

    shapeIter->second = SalaShape(line);

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(shaperef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    auto rowid = static_cast<size_t>(std::distance(m_shapes.begin(), shapeIter));
    AttributeRow &row = m_attributes->getRow(AttributeKey(shapeIter->first));
    // change connections:
    if (m_hasgraph) {

        const std::vector<size_t> oldconnections = m_connectors[rowid].connections;

        auto connCol = m_attributes->getOrInsertLockedColumn("Connectivity");

        if (isAxialMap()) {
            // line connections optimised for line-line intersection
            m_connectors[rowid].connections = getLineConnections(
                shaperef, TOLERANCE_B * std::max(m_region.height(), m_region.width()));
        } else {
            m_connectors[rowid].connections = getShapeConnections(
                shaperef, TOLERANCE_B * std::max(m_region.height(), m_region.width()));
        }

        std::vector<size_t> &newconnections = m_connectors[rowid].connections;
        row.setValue(connCol, static_cast<float>(newconnections.size()));
        if (isAxialMap()) {
            auto lengCol = m_attributes->getOrInsertLockedColumn("Line Length");
            row.setValue(lengCol, static_cast<float>(
                                      genlib::getMapAtIndex(m_shapes, rowid)->second.getLength()));
        }

        // now go through our old connections, and remove ourself:
        for (auto oldconnection : oldconnections) {
            if (oldconnection != rowid) { // <- exclude self!
                auto &connections = m_connectors[static_cast<size_t>(oldconnection)].connections;
                genlib::findAndErase(connections, rowid);
                auto &oldConnectionRow = getAttributeRowFromShapeIndex(oldconnection);
                oldConnectionRow.incrValue(connCol, -1.0f);
            }
        }
        // now go through our new connections, and add ourself:
        for (auto newconnection : m_connectors[rowid].connections) {
            if (newconnection != rowid) { // <- exclude self!
                genlib::insert_sorted(m_connectors[newconnection].connections, rowid);
                auto &newConnectionRow = getAttributeRowFromShapeIndex(newconnection);
                newConnectionRow.incrValue(connCol);
            }
        }
        // now check any unlinks still exist in our newconnections are unlinked
        // again (argh...)
        for (auto revIter = m_unlinks.rbegin(); revIter != m_unlinks.rend(); ++revIter) {
            std::optional<size_t> connb = std::nullopt;
            if (revIter->a == rowid)
                connb = revIter->b;
            else if (revIter->b == rowid)
                connb = revIter->a;
            if (connb.has_value()) {
                if (std::find(newconnections.begin(), newconnections.end(), connb) ==
                    newconnections.end()) {
                    // no longer required:
                    m_unlinks.erase(std::next(revIter).base());
                } else {
                    // enforce:
                    genlib::findAndErase(newconnections, connb.value());
                    genlib::findAndErase(m_connectors[connb.value()].connections, rowid);
                    auto &connbRow = getAttributeRowFromShapeIndex(connb.value());
                    connbRow.incrValue(connCol, -1.0f);
                    row.incrValue(connCol, -1.0f);
                }
            }
        }
        // now check any links are actually required (argh...)
        for (auto revIter = m_links.rbegin(); revIter != m_links.rend(); ++revIter) {
            std::optional<size_t> connb = std::nullopt;
            if (revIter->a == rowid)
                connb = revIter->b;
            else if (revIter->b == rowid)
                connb = revIter->a;
            if (connb.has_value()) {
                if (std::find(newconnections.begin(), newconnections.end(), connb) !=
                    newconnections.end()) {
                    // no longer required:
                    m_links.erase(std::next(revIter).base());
                } else {
                    // enforce:
                    genlib::insert_sorted(newconnections, connb.value());
                    genlib::insert_sorted(m_connectors[connb.value()].connections, rowid);
                    auto &connbRow = getAttributeRowFromShapeIndex(connb.value());
                    connbRow.incrValue(connCol);
                    row.incrValue(connCol);
                }
            }
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// some functions to make a polygon from the UI

int ShapeMap::polyBegin(const Line4f &line) {
    // add geometry
    bool boundsGood = true;
    if (!(m_region.contains_touch(line.start()) && m_region.contains_touch(line.end()))) {
        boundsGood = false;
        init(m_shapes.size(), line);
    }

    int newShapeRef = getNextShapeKey();
    m_shapes.insert(std::make_pair(newShapeRef, SalaShape(line)));
    m_shapes.rbegin()->second.m_centroid = line.getCentre();

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(newShapeRef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    // insert into attributes
    m_attributes->addRow(AttributeKey(newShapeRef));
    // would usually set attributes here, but actually, really want
    // to set the attributes only when the user completes the drawing

    // change connections:
    if (m_hasgraph) {
        // dummy for now to ensure there is a row in the connector table
        // so all indices match...
        m_connectors.push_back(Connector());
    }

    // set undo counter:
    m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_CREATED, newShapeRef));

    return newShapeRef;
}

bool ShapeMap::polyAppend(int shapeRef, const Point2f &point) {
    // don't do anything too complex:
    SalaShape &firstShape = m_shapes.rbegin()->second;

    // check you can actually do this first
    if (!(firstShape.isLine() || firstShape.isPolyLine())) {
        return false;
    }

    // junk the old shape pixels:
    removePolyPixels(shapeRef);

    bool boundsGood = true;
    if (!m_region.contains_touch(point)) {
        boundsGood = false;
        init(m_shapes.size(), Region4f(point, point));
    }

    if (firstShape.m_type == SalaShape::SHAPE_LINE) {
        // convert it to a poly line:
        firstShape.m_type = SalaShape::SHAPE_POLY;
        firstShape.points.push_back(firstShape.m_region.t_start());
        firstShape.points.push_back(firstShape.m_region.t_end());
    }
    // add new point:
    firstShape.points.push_back(point);

    if (boundsGood) {
        // note: also sets polygon bounding box:
        makePolyPixels(shapeRef);
    } else {
        // pixelate all polys in the pixel new structure:
        for (const auto &shape : m_shapes) {
            makePolyPixels(shape.first);
        }
    }

    firstShape.setCentroidAreaPerim();

    return true;
}

bool ShapeMap::polyClose(int shapeRef) {
    // don't do anything too complex:
    SalaShape &shape = m_shapes.rbegin()->second;

    // check you can actually do this first
    if (!shape.isPolyLine()) {
        return false;
    }

    // junk the old shape pixels:
    removePolyPixels(shapeRef);

    shape.m_type |= SalaShape::SHAPE_CLOSED;

    makePolyPixels(shapeRef);

    return true;
}

bool ShapeMap::polyCancel(int shapeRef) {
    // don't do anything too complex:
    SalaShape &shape = m_shapes.rbegin()->second;

    // check you can actually do this first
    if (!(shape.isLine() || shape.isPolyLine())) {
        return false;
    }

    m_undobuffer.pop_back();
    removeShape(shapeRef, true);

    return true;
}

void ShapeMap::removeShape(int shaperef, bool undoing) {
    // remove shape from four keys: the pixel grid, the poly list, the attributes
    // and the connections
    removePolyPixels(shaperef); // done first, as all interface references use this list

    auto shapeIter = m_shapes.find(shaperef);
    if (shapeIter == m_shapes.end()) {
        throw genlib::RuntimeException("Shape with ref " + std::to_string(shaperef) +
                                       " not found when trying to remove it");
    }
    size_t rowid = static_cast<size_t>(std::distance(m_shapes.begin(), shapeIter));

    if (!undoing) { // <- if not currently undoing another event, then add to the
                    // undo buffer:
        m_undobuffer.push_back(SalaEvent(SalaEvent::SALA_DELETED, shaperef));
        m_undobuffer.back().geometry = shapeIter->second;
    }

    if (m_hasgraph) {
        // note that the connections have no key for speed when processing,
        // we rely on the index order matching the index order of the shapes
        // and the attributes, and simply change all references (ick!)
        auto connCol = m_attributes->getColumnIndex("Connectivity");

        // TODO: Replace with iterators
        for (size_t i = m_connectors.size() - 1; static_cast<int>(i) != -1; i--) {
            if (i == rowid) {
                continue; // it's going to be removed anyway
            }
            for (size_t j = m_connectors[i].connections.size() - 1; static_cast<int>(j) != -1;
                 j--) {
                if (m_connectors[i].connections[j] == rowid) {
                    m_connectors[i].connections.erase(m_connectors[i].connections.begin() +
                                                      static_cast<int>(j));
                    // getColumnIndex will throw if the column is not found
                    // if (conn_col != -1) {
                    auto &row = getAttributeRowFromShapeIndex(i);
                    row.incrValue(connCol, -1.0f);
                    // }
                } else if (m_connectors[i].connections[j] > rowid) {
                    m_connectors[i].connections[j] -= 1;
                }
            }
            // note, you cannot delete from a segment map, it's just too messy!
        }

        m_connectors.erase(m_connectors.begin() + static_cast<int>(rowid));

        // take out explicit links and unlinks (note, undo won't restore these):
        for (auto revIter = m_links.rbegin(); revIter != m_links.rend(); ++revIter) {
            if (revIter->a == rowid || revIter->b == rowid) {
                m_links.erase(std::next(revIter).base());
            } else {
                if (revIter->a > rowid)
                    revIter->a -= 1;
                if (revIter->b > rowid)
                    revIter->b -= 1;
            }
        }
        for (auto revIter = m_unlinks.rbegin(); revIter != m_unlinks.rend(); ++revIter) {
            if (revIter->a == rowid || revIter->b == rowid) {
                m_unlinks.erase(std::next(revIter).base());
            } else {
                if (revIter->a > rowid)
                    revIter->a -= 1;
                if (revIter->b > rowid)
                    revIter->b -= 1;
            }
        }
    }

    if (shapeIter != m_shapes.end()) {
        shapeIter = m_shapes.erase(shapeIter);
    }
    // n.b., shaperef should have been used to create the row in the first place:
    const AttributeKey shapeRefKey(shaperef);
    m_attributes->removeRow(shapeRefKey);
}

void ShapeMap::undo() {
    if (m_undobuffer.size() == 0) {
        return;
    }

    SalaEvent &event = m_undobuffer.back();

    if (event.action == SalaEvent::SALA_CREATED) {

        removeShape(event.shapeRef,
                    true); // <- note, must tell remove shape it's an undo, or it
                           // will add this remove to the undo stack!

    } else if (event.action == SalaEvent::SALA_DELETED) {

        makeShape(event.geometry, event.shapeRef);
        auto rowIt = m_shapes.find(event.shapeRef);

        if (rowIt != m_shapes.end() && m_hasgraph) {
            auto rowid = static_cast<size_t>(std::distance(m_shapes.begin(), rowIt));
            auto &row = m_attributes->getRow(AttributeKey(event.shapeRef));
            // redo connections... n.b. TO DO this is intended to use the slower "any
            // connection" method, so it can handle any sort of graph
            // ...but that doesn't exist yet, so for the moment do lines:
            //
            // insert new connector at the row:
            m_connectors[rowid] = Connector();
            //
            // now go through all connectors, ensuring they're reindexed above this
            // one: Argh!  ...but, remember the reason we're doing this is for fast
            // processing elsewhere
            // -- this is a user triggered *undo*, they'll just have to wait:
            for (size_t i = 0; i < m_connectors.size(); i++) {
                for (size_t j = 0; j < m_connectors[i].connections.size(); j++) {
                    if (m_connectors[i].connections[j] >= rowid) {
                        m_connectors[i].connections[j] += 1;
                    }
                }
            }
            // it gets worse, the links and unlinks will also be all over the shop due
            // to the inserted row:
            size_t j;
            for (j = 0; j < m_links.size(); j++) {
                if (m_links[j].a >= rowid)
                    m_links[j].a += 1;
                if (m_links[j].b >= rowid)
                    m_links[j].b += 1;
            }
            for (j = 0; j < m_unlinks.size(); j++) {
                if (m_unlinks[j].a >= rowid)
                    m_unlinks[j].a += 1;
                if (m_unlinks[j].b >= rowid)
                    m_unlinks[j].b += 1;
            }
            //
            // calculate this line's connections
            m_connectors[rowid].connections = getLineConnections(
                event.shapeRef, TOLERANCE_B * std::max(m_region.height(), m_region.width()));
            // update:
            auto connCol = m_attributes->getOrInsertLockedColumn("Connectivity");
            row.setValue(connCol, static_cast<float>(m_connectors[rowid].connections.size()));
            //
            if (event.geometry.isLine()) {
                auto lengCol = m_attributes->getOrInsertLockedColumn("Line Length");
                row.setValue(
                    lengCol,
                    static_cast<float>(genlib::getMapAtIndex(m_shapes, rowid)->second.getLength()));
            }
            //
            // now go through our connections, and add ourself:
            const auto &connections = m_connectors[rowid].connections;
            for (auto connection : connections) {
                if (connection != rowid) { // <- exclude self!
                    genlib::insert_sorted(m_connectors[connection].connections, rowid);
                    getAttributeRowFromShapeIndex(connection).incrValue(connCol);
                }
            }
        }
    } else if (event.action == SalaEvent::SALA_MOVED) {

        moveShape(event.shapeRef, event.geometry.getLine(),
                  true); // <- note, must tell remove shape it's an undo, or it will
                         // add this remove to the undo stack!
    }

    m_undobuffer.pop_back();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::makePolyPixels(int polyref) {
    // first add into pixels, and ensure you have a bl, tr for the set (useful for
    // testing later)
    auto shapeIter = m_shapes.find(polyref);
    if (shapeIter == m_shapes.end()) {
        throw genlib::RuntimeException("Shape " + std::to_string(polyref) +
                                       " not found when making poly pixels");
    }
    SalaShape &poly = shapeIter->second;
    if (poly.isClosed()) {
        ShapeRef shapeRef = ShapeRef(static_cast<unsigned int>(polyref));
        std::map<int, int> relations;
        for (size_t k = 0; k < poly.points.size(); k++) {
            int nextk = static_cast<int>((k + 1) % poly.points.size());
            Line4f li(poly.points[k], poly.points[static_cast<size_t>(nextk)]);
            if (k == 0) {
                poly.m_region = li;
            } else {
                poly.m_region = poly.m_region.runion(li);
            }
            PixelRefVector pixels = pixelateLine(li);
            // debug
            // int duplicate_shaperefs = 0;
            // end debug
            for (size_t i = 0; i < pixels.size(); i++) {
                PixelRef pix = pixels[i];
                std::vector<ShapeRef> &pixShapes =
                    m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
                auto it = genlib::findBinary(pixShapes, shapeRef);
                if (it == pixShapes.end()) {
                    pixShapes.push_back(shapeRef);
                    it = pixShapes.end() - 1;
                }
                it->polyrefs.push_back(static_cast<short>(k));
                relations.insert(std::make_pair(pixels[i], ShapeRef::SHAPE_EDGE));
            }
        }
        // erase joined sides, and look for min:
        PixelRef minpix = NoPixel;
        for (auto &relation : relations) {
            PixelRef pix = relation.first;
            PixelRef nextpix;
            nextpix = pix.right();
            if (includes(nextpix)) {
                auto &pixShapes =
                    m_pixelShapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (genlib::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_R;
                }
            }
            nextpix = pix.up();
            if (includes(nextpix)) {
                auto &pixShapes =
                    m_pixelShapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (genlib::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_T;
                }
            }
            nextpix = pix.down();
            if (includes(nextpix)) {
                auto &pixShapes =
                    m_pixelShapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (genlib::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_B;
                }
            }
            nextpix = pix.left();
            if (includes(nextpix)) {
                auto &pixShapes =
                    m_pixelShapes(static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                if (genlib::findBinary(pixShapes, shapeRef) != pixShapes.end()) {
                    relation.second &= ~ShapeRef::SHAPE_L;
                }
            }
            if ((relation.second & (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) ==
                (ShapeRef::SHAPE_B | ShapeRef::SHAPE_L)) {
                if ((minpix == NoPixel) || (relation.first < static_cast<int>(minpix))) {
                    minpix = relation.first;
                }
            }
        }
        shapePixelBorder(relations, polyref, ShapeRef::SHAPE_L, minpix, minpix, true);
        // go through any that aren't on the outer border: this will be internal
        // edges, and will cause problems for point in polygon algorithms!

        for (auto &relation : relations) {
            PixelRef pix = relation.first;
            std::vector<ShapeRef> &pixShapes =
                m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            const auto iter = genlib::findBinary(pixShapes, shapeRef);
            if (iter == pixShapes.end())
                throw new genlib::RuntimeException("Poly reference not found");
            uint8_t &tags = iter->tags;
            if (tags == 0x00) {
                tags |= ShapeRef::SHAPE_INTERNAL_EDGE;
            }
        }
        // now, any remaining tags are internal sides, and need to be cleared
        // through fill we could go either direction, but we just go left to right:
        for (auto &relation : relations) {
            PixelRef pix = relation.first;
            if (relation.second & ShapeRef::SHAPE_R) {
                bool lastWasNotFound = true;
                while (lastWasNotFound) {
                    PixelRef nextpix = pix.right();
                    if (!includes(nextpix)) {
                        // this shouldn't happen
                        break;
                    }
                    // returns -1 if cannot add due to already existing:
                    lastWasNotFound = false;
                    std::vector<ShapeRef> &pixelShapes = m_pixelShapes(
                        static_cast<size_t>(nextpix.y), static_cast<size_t>(nextpix.x));
                    const auto it = genlib::findBinary(pixelShapes, shapeRef);
                    if (it == pixelShapes.end()) {
                        lastWasNotFound = true;
                        pixelShapes.push_back(
                            ShapeRef(static_cast<unsigned int>(polyref), ShapeRef::SHAPE_CENTRE));
                    }
                    pix = nextpix;
                }
            }
        }
        // Done...! This polygon is registered in the pixel polygon structure
    } else {
        // Open shapes much easier!
        switch (poly.m_type & SalaShape::SHAPE_TYPE) {
        case SalaShape::SHAPE_POINT: {
            PixelRef pix = pixelate(poly.m_centroid);
            std::vector<ShapeRef> &pixShapes =
                m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            const auto it =
                genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
            if (it == pixShapes.end()) {
                pixShapes.push_back(
                    ShapeRef(static_cast<unsigned int>(polyref), ShapeRef::SHAPE_OPEN));
            }
        } break;
        case SalaShape::SHAPE_LINE: {
            ShapeRef shapeRef = ShapeRef(static_cast<unsigned int>(polyref));
            PixelRefVector pixels = pixelateLine(poly.m_region);
            for (size_t i = 0; i < pixels.size(); i++) {
                PixelRef pix = pixels[i];
                std::vector<ShapeRef> &pixShapes =
                    m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
                const auto it = genlib::findBinary(pixShapes, shapeRef);
                if (it == pixShapes.end()) {
                    pixShapes.push_back(
                        ShapeRef(static_cast<unsigned int>(polyref), ShapeRef::SHAPE_OPEN));
                }
            }
        } break;
        case SalaShape::SHAPE_POLY: {
            ShapeRef shapeRef = ShapeRef(static_cast<unsigned int>(polyref));
            for (size_t k = 0; k < poly.points.size() - 1; k++) {
                auto nextk = (k + 1);
                Line4f li(poly.points[k], poly.points[nextk]);
                if (k == 0) {
                    poly.m_region = li;
                } else {
                    poly.m_region = poly.m_region.runion(li);
                }
                PixelRefVector pixels = pixelateLine(li);
                for (size_t i = 0; i < pixels.size(); i++) {
                    PixelRef pix = pixels[i];
                    std::vector<ShapeRef> &pixShapes =
                        m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
                    auto it = genlib::findBinary(pixShapes, shapeRef);
                    if (it == pixShapes.end()) {
                        pixShapes.push_back(
                            ShapeRef(static_cast<unsigned int>(polyref), ShapeRef::SHAPE_OPEN));
                        it = pixShapes.end() - 1;
                    }
                    it->polyrefs.push_back(static_cast<short>(k));
                }
            }
            break;
        }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::shapePixelBorder(std::map<int, int> &relations, int polyref, int side,
                                PixelRef currpix, PixelRef minpix, bool first) {
    if (!first && currpix == minpix && side == ShapeRef::SHAPE_L) {
        // looped:
        return;
    }
    auto relation = relations.find(currpix);
    if (relation != relations.end() && (relation->second & side)) {
        std::vector<ShapeRef> &pixShapes =
            m_pixelShapes(static_cast<size_t>(currpix.y), static_cast<size_t>(currpix.x));
        const auto iter =
            genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
        if (iter == pixShapes.end())
            throw new genlib::RuntimeException("Poly reference not found");
        iter->tags |= static_cast<uint8_t>(side);
        relation->second &= ~side; // <- clear to check all have been done later
        side <<= 1;
        if (side > ShapeRef::SHAPE_T) {
            side = ShapeRef::SHAPE_L;
        }
        shapePixelBorder(relations, polyref, side, currpix, minpix, false);
    } else {
        currpix.move(static_cast<int8_t>(moveDir(side)));
        side >>= 1;
        if (side < ShapeRef::SHAPE_L) {
            side = ShapeRef::SHAPE_T;
        }
        shapePixelBorder(relations, polyref, side, currpix, minpix, false);
    }
}

// note that this is almost exactly the same as shapePixelBorder
void ShapeMap::pointPixelBorder(const PointMap &pointmap, std::map<int, int> &relations,
                                SalaShape &poly, int side, PixelRef currpix, PixelRef minpix,
                                bool first) {
    if (!first && currpix == minpix && side == ShapeRef::SHAPE_L) {
        // looped:
        return;
    }
    auto relation = relations.find(currpix);
    if (relation != relations.end() && (relation->second & side)) {
        poly.points.push_back(pointmap.depixelate(currpix) + pointOffset(pointmap, side));
        relation->second &= ~side; // <- clear to check all have been done later
        side <<= 1;
        if (side > ShapeRef::SHAPE_T) {
            side = ShapeRef::SHAPE_L;
        }
        pointPixelBorder(pointmap, relations, poly, side, currpix, minpix, false);
    } else {
        currpix.move(static_cast<int8_t>(moveDir(side)));
        side >>= 1;
        if (side < ShapeRef::SHAPE_L) {
            side = ShapeRef::SHAPE_T;
        }
        pointPixelBorder(pointmap, relations, poly, side, currpix, minpix, false);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void ShapeMap::removePolyPixels(int polyref) {
    auto shapeIter = m_shapes.find(polyref);
    if (shapeIter == m_shapes.end()) {
        return;
    }
    SalaShape &poly = shapeIter->second;
    if (poly.isClosed()) {
        // easiest just to use scan lines to find internal pixels rather than trace
        // a complex border:
        PixelRef minpix = pixelate(poly.m_region.bottomLeft);
        PixelRef maxpix = pixelate(poly.m_region.topRight);
        for (int x = minpix.x; x <= maxpix.x; x++) {
            for (int y = minpix.y; y <= maxpix.y; y++) {
                std::vector<ShapeRef> &pixShapes =
                    m_pixelShapes(static_cast<size_t>(y), static_cast<size_t>(x));
                const auto it =
                    genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
                if (it != pixShapes.end())
                    pixShapes.erase(it);
            }
        }
    } else {
        // open shapes easier still, as no need to find internal pixels:
        switch (poly.m_type & SalaShape::SHAPE_TYPE) {
        case SalaShape::SHAPE_POINT: {
            PixelRef pix = pixelate(poly.m_centroid);
            std::vector<ShapeRef> &pixShapes =
                m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            const auto it =
                genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
            if (it != pixShapes.end())
                pixShapes.erase(it);
        } break;
        case SalaShape::SHAPE_LINE: {
            PixelRefVector list = pixelateLine(poly.m_region);
            for (size_t i = 0; i < list.size(); i++) {
                std::vector<ShapeRef> &pixShapes =
                    m_pixelShapes(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
                const auto it =
                    genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
                if (it != pixShapes.end())
                    pixShapes.erase(it);
            }
        } break;
        case SalaShape::SHAPE_POLY:
            for (size_t k = 0; k < poly.points.size() - 1; k++) {
                size_t nextk = (k + 1);
                Line4f li(poly.points[k], poly.points[nextk]);
                PixelRefVector list = pixelateLine(li);
                for (size_t i = 0; i < list.size(); i++) {
                    std::vector<ShapeRef> &pixShapes = m_pixelShapes(
                        static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
                    const auto it =
                        genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
                    if (it != pixShapes.end())
                        pixShapes.erase(it);
                }
            }
            break;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

int ShapeMap::moveDir(int side) {
    int dir = PixelRef::NODIR;
    switch (side) {
    case ShapeRef::SHAPE_L:
        dir = PixelRef::NEGHORIZONTAL;
        break;
    case ShapeRef::SHAPE_B:
        dir = PixelRef::NEGVERTICAL;
        break;
    case ShapeRef::SHAPE_R:
        dir = PixelRef::HORIZONTAL;
        break;
    case ShapeRef::SHAPE_T:
        dir = PixelRef::VERTICAL;
        break;
    }
    return dir;
}

Point2f ShapeMap::pointOffset(const PointMap &pointmap, int side) {
    Point2f p;
    switch (side) {
    case ShapeRef::SHAPE_L:
        p = Point2f(-pointmap.getSpacing() / 2, 0.0);
        break;
    case ShapeRef::SHAPE_B:
        p = Point2f(0.0, -pointmap.getSpacing() / 2);
        break;
    case ShapeRef::SHAPE_R:
        p = Point2f(pointmap.getSpacing() / 2, 0.0);
        break;
    case ShapeRef::SHAPE_T:
        p = Point2f(0.0, pointmap.getSpacing() / 2);
        break;
    }
    return p;
}

// Point in poly testing (returns topmost displayed poly)

int ShapeMap::pointInPoly(const Point2f &p) const {
    if (!m_region.contains(p)) {
        return -1;
    }
    std::vector<size_t> testedshapes;
    PixelRef pix = pixelate(p);
    const std::vector<ShapeRef> &shapes =
        m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    int drawlast = -1;
    int draworder = -1;

    for (const ShapeRef &shape : shapes) {
        auto iter = genlib::findBinary(testedshapes, shape.shapeRef);
        if (iter != testedshapes.end()) {
            continue;
        }
        testedshapes.insert(iter, shape.shapeRef);

        auto shapeindex = testPointInPoly(p, shape);

        // if there's a shapeindex, then add:
        int currentDrawOrder = static_cast<int>(
            m_attribHandle->findInIndex(AttributeKey(static_cast<int>(shape.shapeRef))));
        if (shapeindex.has_value() && currentDrawOrder > draworder) {
            drawlast = static_cast<int>(shapeindex.value());
            draworder = currentDrawOrder;
        }
    }
    return drawlast;
}

// Point in specific poly (by reference)

bool ShapeMap::pointInPoly(const Point2f &p, int polyref) const {
    PixelRef pix = pixelate(p);
    const std::vector<ShapeRef> &shapes =
        m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    const auto iter = genlib::findBinary(shapes, ShapeRef(static_cast<unsigned int>(polyref)));
    if (iter != shapes.end()) {
        return (testPointInPoly(p, *iter).has_value());
    }
    return false;
}

// similar to above, but builds a list

std::vector<size_t> ShapeMap::pointInPolyList(const Point2f &p) const {
    std::vector<size_t> shapeindexlist;
    if (!m_region.contains(p)) {
        return shapeindexlist;
    }
    std::vector<size_t> testedshapes;
    PixelRef pix = pixelate(p);
    const std::vector<ShapeRef> &shapes =
        m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    for (const ShapeRef &shape : shapes) {
        auto iter = genlib::findBinary(testedshapes, shape.shapeRef);
        if (iter != testedshapes.end()) {
            continue;
        }
        testedshapes.insert(iter, shape.shapeRef);

        auto shapeindex = testPointInPoly(p, shape);

        // if there's a shapeindex, then add (note it is an add -- you may be passed
        // a list again to expand)
        if (shapeindex.has_value()) {
            shapeindexlist.push_back(shapeindex.value());
        }
    }
    std::sort(shapeindexlist.begin(), shapeindexlist.end());
    return shapeindexlist;
}

// note, lineref is only used as an "exclude self" test when called from
// getShapeConnections
std::vector<size_t> ShapeMap::lineInPolyList(const Line4f &liOrig, std::optional<size_t> lineref,
                                             double tolerance) const {
    std::vector<size_t> shapeindexlist;
    if (!m_region.intersects(liOrig)) {
        return shapeindexlist;
    }
    Line4f li = liOrig;
    if (!m_region.contains(li.start()) || !m_region.contains(li.end())) {
        li.crop(m_region);
    }

    shapeindexlist = pointInPolyList(li.start());
    std::vector<size_t> endShapeIndexList = pointInPolyList(li.end());
    shapeindexlist.insert(shapeindexlist.end(), endShapeIndexList.begin(), endShapeIndexList.end());

    // only now pixelate and test for any other shapes:
    PixelRefVector list = pixelateLine(li);
    for (size_t i = 0; i < list.size(); i++) {
        PixelRef pix = list[i];
        if (includes(pix)) {
            const std::vector<ShapeRef> &shapes =
                m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
            for (const ShapeRef &shape : shapes) {
                // slow to do this as it can repeat -- really need to use a linetest
                // like structure to avoid retest of polygon lines
                if (lineref.has_value() && shape.shapeRef != lineref.value() &&
                    shape.tags & (ShapeRef::SHAPE_EDGE | ShapeRef::SHAPE_INTERNAL_EDGE |
                                  ShapeRef::SHAPE_OPEN)) {
                    auto shapeIter = m_shapes.find(static_cast<int>(shape.shapeRef));
                    if (shapeIter == m_shapes.end()) {
                        throw genlib::RuntimeException(
                            "Shape " + std::to_string(shape.shapeRef) +
                            " not found when checking if line in poly list");
                    }
                    const SalaShape &poly = shapeIter->second;
                    switch (poly.m_type & (SalaShape::SHAPE_LINE | SalaShape::SHAPE_POLY)) {
                    case SalaShape::SHAPE_LINE:
                        if (li.Region4f::intersects(poly.m_region)) {
                            // note: in this case m_region is stored as a line:
                            if (li.Line4f::intersects(poly.m_region, tolerance)) {
                                shapeindexlist.push_back(static_cast<size_t>(
                                    std::distance(m_shapes.begin(), shapeIter)));
                            }
                        }
                        break;
                    case SalaShape::SHAPE_POLY: {
                        for (size_t k = 0; k < shape.polyrefs.size(); k++) {
                            Line4f lineb =
                                Line4f(poly.points[static_cast<size_t>(shape.polyrefs[k])],
                                       poly.points[(static_cast<size_t>(shape.polyrefs[k] + 1) %
                                                    poly.points.size())]);
                            if (li.Region4f::intersects(lineb)) {
                                if (li.Line4f::intersects(lineb, tolerance)) {
                                    shapeindexlist.push_back(static_cast<size_t>(
                                        std::distance(m_shapes.begin(), shapeIter)));
                                }
                            }
                        }
                    } break;
                    default:
                        break;
                    }
                }
            }
        }
    }
    std::sort(shapeindexlist.begin(), shapeindexlist.end());
    return shapeindexlist;
}

std::vector<size_t> ShapeMap::polyInPolyList(int polyref, double tolerance) const {
    std::vector<size_t> shapeindexlist;
    auto shapeRefIter = m_shapes.find(polyref);
    if (shapeRefIter == m_shapes.end()) {
        return shapeindexlist;
    }
    const SalaShape &poly = shapeRefIter->second;
    if (poly.isClosed()) { // <- it ought to be, you shouldn't be using this
                           // function if not!
        std::vector<size_t> testedlist;
        // easiest just to use scan lines to find internal pixels rather than trace
        // a complex border:
        PixelRef minpix = pixelate(poly.m_region.bottomLeft);
        PixelRef maxpix = pixelate(poly.m_region.topRight);
        // pass one: shape centre of either object coincident automatically adds
        int x;
        for (x = minpix.x; x <= maxpix.x; x++) {
            for (int y = minpix.y; y <= maxpix.y; y++) {
                const std::vector<ShapeRef> &pixShapes =
                    m_pixelShapes(static_cast<size_t>(y), static_cast<size_t>(x));
                const auto pixIter =
                    genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
                if (pixIter != pixShapes.end()) {
                    // this has us in it, now looked through everything else:
                    for (const ShapeRef &shapeRef : pixShapes) {
                        if (*pixIter != shapeRef && ((pixIter->tags & ShapeRef::SHAPE_CENTRE) ||
                                                     (shapeRef.tags & ShapeRef::SHAPE_CENTRE))) {
                            auto iter = genlib::findBinary(testedlist, shapeRef.shapeRef);
                            if (iter == testedlist.end()) {
                                testedlist.insert(iter, shapeRef.shapeRef);
                                auto shapeIdx = genlib::findIndexFromKey(
                                    m_shapes, static_cast<int>(shapeRef.shapeRef));
                                shapeindexlist.push_back(static_cast<size_t>(shapeIdx));
                            }
                        }
                    }
                }
            }
        }
        // that was the easy bit... now, pass 2, for non centre things:
        for (x = minpix.x; x <= maxpix.x; x++) {
            for (int y = minpix.y; y <= maxpix.y; y++) {
                const std::vector<ShapeRef> &pixShapes =
                    m_pixelShapes(static_cast<size_t>(y), static_cast<size_t>(x));
                const auto pixIter =
                    genlib::findBinary(pixShapes, ShapeRef(static_cast<unsigned int>(polyref)));
                if (pixIter != pixShapes.end()) {
                    const ShapeRef &shaperef = *pixIter;
                    if ((shaperef.tags & ShapeRef::SHAPE_CENTRE) == 0) {
                        // this has us in it, now looked through everything else:
                        for (auto &shaperefb : pixShapes) {
                            auto iter = genlib::findBinary(testedlist, shaperefb.shapeRef);
                            if (shaperef != shaperefb && iter == testedlist.end()) {
                                auto shapeIter =
                                    m_shapes.find(static_cast<int>(shaperefb.shapeRef));
                                if (shapeIter == m_shapes.end()) {
                                    throw genlib::RuntimeException(
                                        "Shape " + std::to_string(shaperefb.shapeRef) +
                                        " not found when checking if point in poly list");
                                }
                                size_t indexb =
                                    static_cast<size_t>(std::distance(m_shapes.begin(), shapeIter));
                                const SalaShape &polyb = shapeIter->second;
                                if (polyb.isPoint()) {
                                    if (testPointInPoly(polyb.getPoint(), shaperef).has_value()) {
                                        shapeindexlist.push_back(static_cast<size_t>(indexb));
                                    }
                                } else if (polyb.isLine()) {
                                    if (testPointInPoly(polyb.getLine().start(), shaperef)
                                            .has_value() ||
                                        testPointInPoly(polyb.getLine().end(), shaperef)
                                            .has_value()) {
                                        testedlist.insert(iter, shaperefb.shapeRef);
                                        shapeindexlist.push_back(static_cast<size_t>(indexb));
                                    } else {
                                        for (size_t k = 0; k < shaperef.polyrefs.size(); k++) {
                                            Line4f line =
                                                Line4f(poly.points[static_cast<size_t>(
                                                           shaperef.polyrefs[k])],
                                                       poly.points[(static_cast<size_t>(
                                                                        shaperef.polyrefs[k] + 1) %
                                                                    poly.points.size())]);
                                            if (line.Region4f::intersects(polyb.getLine())) {
                                                if (line.Line4f::intersects(polyb.getLine(),
                                                                            tolerance)) {
                                                    testedlist.insert(iter, shaperefb.shapeRef);
                                                    shapeindexlist.push_back(
                                                        static_cast<size_t>(indexb));
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                } else if (polyb.isPolyLine()) {
                                    if (testPointInPoly(polyb.points[static_cast<size_t>(
                                                            shaperefb.polyrefs[0])],
                                                        shaperef)
                                            .has_value()) {
                                        testedlist.insert(iter, shaperefb.shapeRef);
                                        shapeindexlist.push_back(static_cast<size_t>(indexb));
                                    } else {
                                        for (size_t k = 0; k < shaperef.polyrefs.size(); k++) {
                                            for (size_t kk = 0; kk < shaperefb.polyrefs.size();
                                                 kk++) {
                                                Line4f line = Line4f(
                                                    poly.points[static_cast<size_t>(
                                                        shaperef.polyrefs[k])],
                                                    poly.points[(static_cast<size_t>(
                                                                     shaperef.polyrefs[k] + 1) %
                                                                 poly.points.size())]);
                                                Line4f lineb = Line4f(
                                                    polyb.points[static_cast<size_t>(
                                                        shaperefb.polyrefs[kk])],
                                                    polyb.points[(static_cast<size_t>(
                                                                      shaperefb.polyrefs[kk] + 1) %
                                                                  polyb.points.size())]);
                                                if (line.Region4f::intersects(lineb)) {
                                                    if (line.Line4f::intersects(lineb, tolerance)) {
                                                        auto iterInternal = genlib::findBinary(
                                                            testedlist, shaperefb.shapeRef);
                                                        if (iterInternal == testedlist.end()) {
                                                            testedlist.insert(iterInternal,
                                                                              shaperefb.shapeRef);
                                                            shapeindexlist.push_back(
                                                                static_cast<size_t>(indexb));
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    // poly to poly, ick!
                                    // first test one entirely inside the other
                                    // any point at all will suffice to check this: however, we
                                    // need to check that the polyref point *itself* is within the
                                    // pixel, not just part of the line associated with it...
                                    if ((pixelate(polyb.points[static_cast<size_t>(
                                             shaperefb.polyrefs[0])]) ==
                                             PixelRef(static_cast<short>(x),
                                                      static_cast<short>(y)) &&
                                         testPointInPoly(polyb.points[static_cast<size_t>(
                                                             shaperefb.polyrefs[0])],
                                                         shaperef)
                                             .has_value()) ||
                                        (pixelate(poly.points[static_cast<size_t>(
                                             shaperef.polyrefs[0])]) ==
                                             PixelRef(static_cast<short>(x),
                                                      static_cast<short>(y)) &&
                                         testPointInPoly(
                                             poly.points[static_cast<size_t>(shaperef.polyrefs[0])],
                                             shaperefb)
                                             .has_value())) {
                                        testedlist.insert(iter, shaperefb.shapeRef);
                                        shapeindexlist.push_back(static_cast<size_t>(indexb));
                                    } else {
                                        // now check crossing
                                        bool breakit = false;
                                        for (size_t k = 0; k < shaperef.polyrefs.size() && !breakit;
                                             k++) {
                                            for (size_t kk = 0; kk < shaperefb.polyrefs.size();
                                                 kk++) {
                                                Line4f line = Line4f(
                                                    poly.points[static_cast<size_t>(
                                                        shaperef.polyrefs[k])],
                                                    poly.points[(static_cast<size_t>(
                                                                     shaperef.polyrefs[k] + 1) %
                                                                 poly.points.size())]);
                                                Line4f lineb = Line4f(
                                                    polyb.points[static_cast<size_t>(
                                                        shaperefb.polyrefs[kk])],
                                                    polyb.points[(static_cast<size_t>(
                                                                      shaperefb.polyrefs[kk] + 1) %
                                                                  polyb.points.size())]);
                                                if (line.Region4f::intersects(lineb)) {
                                                    if (line.Line4f::intersects(lineb, tolerance)) {
                                                        testedlist.insert(iter, shaperefb.shapeRef);
                                                        shapeindexlist.push_back(
                                                            static_cast<size_t>(indexb));
                                                        breakit = true;
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    } else {
        throw genlib::RuntimeException("this function is to be used for polygons only");
    }
    std::sort(shapeindexlist.begin(), shapeindexlist.end());
    return shapeindexlist;
}

std::vector<size_t>
ShapeMap::shapeInPolyList(const SalaShape &shape) { // note: no const due to poly in poly testing
    std::vector<size_t> shapeindexlist;
    if (!m_region.intersects(shape.m_region)) {
        // quick test that actually coincident
        return shapeindexlist;
    }
    if (shape.isPoint()) {
        shapeindexlist = pointInPolyList(shape.getPoint());
    } else if (shape.isLine()) {
        shapeindexlist = lineInPolyList(shape.getLine());
    } else if (shape.isPolyLine()) {
        for (size_t i = 1; i < shape.points.size() - 1; i++) {
            Line4f li(shape.points[i], shape.points[i - 1]);
            shapeindexlist = lineInPolyList(li);
        }
    } else {
        // first *add* the poly temporarily (note this may grow pixel set):
        int ref = makePolyShape(shape.points, false,
                                true); // false is closed poly, true is temporary shape
        // do test:
        shapeindexlist = polyInPolyList(ref);
        // clean up:
        removePolyPixels(ref);
        m_shapes.erase(m_shapes.find(ref));
    }
    return shapeindexlist;
}

// helper for point in poly --
// currently needs slight rewrite to avoid problem if point is in line with a
// vertex (counter incremented twice on touching implies not in poly when is)

std::optional<size_t> ShapeMap::testPointInPoly(const Point2f &p, const ShapeRef &shape) const {
    auto shapeIter = m_shapes.end();
    // simplist: in shape centre
    if (shape.tags & ShapeRef::SHAPE_CENTRE) {
        shapeIter = m_shapes.find(static_cast<int>(shape.shapeRef));
    }
    // check not an open shape (cannot be inside)
    else if ((shape.tags & ShapeRef::SHAPE_OPEN) == 0) {
        auto tempShapeIter = m_shapes.find(static_cast<int>(shape.shapeRef));
        if (tempShapeIter == m_shapes.end()) {
            throw genlib::RuntimeException("Shape " + std::to_string(shape.shapeRef) +
                                           " not found when testing if a point is in a polygon");
        }
        const SalaShape &poly = tempShapeIter->second;
        if (poly.m_region.contains_touch(p)) {
            // next simplest, on the outside border:
            int alpha1 = 0;
            int counter1 = 0;
            int parity1 = 0;
            if (shape.tags & ShapeRef::SHAPE_EDGE) {
                // run a test line to the edge:
                if (shape.tags & (ShapeRef::SHAPE_L | ShapeRef::SHAPE_R)) {
                    if (shape.tags & ShapeRef::SHAPE_L) {
                        parity1 = -1;
                    } else if (shape.tags & ShapeRef::SHAPE_R) {
                        parity1 = +1;
                    }
                    for (size_t j = 0; j < shape.polyrefs.size(); j++) {
                        Line4f lineb =
                            Line4f(poly.points[static_cast<size_t>(shape.polyrefs[j])],
                                   poly.points[(static_cast<size_t>(shape.polyrefs[j] + 1) %
                                                poly.points.size())]);
                        if (lineb.bottomLeft.y <= p.y && lineb.topRight.y >= p.y) {
                            // crosses or touches... but we need to check
                            // touching exception:
                            if (lineb.t_start().y == p.y) {
                                if (parity1 * lineb.t_start().x >= parity1 * p.x) {
                                    alpha1 -= 1;
                                    counter1++;
                                }
                            }
                            // the other touching exception
                            else if (lineb.t_end().y == p.y) {
                                if (parity1 * lineb.t_end().x >= parity1 * p.x) {
                                    alpha1 += 1;
                                    // n.b., no counter here
                                }
                            }
                            // at this stage we know the line isn't horizontal, so we can find
                            // the intersection point:
                            else if (parity1 * (lineb.grad(LineAxis::XAXIS) * (p.y - lineb.ay()) +
                                                lineb.ax()) >=
                                     parity1 * p.x) {
                                counter1++;
                            }
                        }
                    }
                } else {
                    if (shape.tags & ShapeRef::SHAPE_B) {
                        parity1 = -1;
                    } else if (shape.tags & ShapeRef::SHAPE_T) {
                        parity1 = +1;
                    }
                    for (size_t j = 0; j < shape.polyrefs.size(); j++) {
                        Line4f lineb =
                            Line4f(poly.points[static_cast<size_t>(shape.polyrefs[j])],
                                   poly.points[(static_cast<size_t>(shape.polyrefs[j] + 1) %
                                                poly.points.size())]);
                        if (lineb.bottomLeft.x <= p.x && lineb.topRight.x >= p.x) {
                            // crosses or touches... but we need to check
                            // touching exception:
                            if (lineb.topRight.x == p.x) {
                                if (parity1 * lineb.by() >= parity1 * p.y) {
                                    alpha1 -= 1;
                                    counter1++;
                                }
                            }
                            // the other touching exception
                            else if (lineb.bottomLeft.x == p.x) {
                                if (parity1 * lineb.ay() >= parity1 * p.y) {
                                    alpha1 += 1;
                                    // n.b., no counter here
                                }
                            }
                            // at this stage we know the line isn't vertical, so we can find
                            // the intersection point:
                            else if (parity1 * (lineb.grad(LineAxis::YAXIS) * (p.x - lineb.ax()) +
                                                lineb.ay()) >=
                                     parity1 * p.y) {
                                counter1++;
                            }
                        }
                    }
                }
                if (counter1 % 2 != 0 && alpha1 == 0) {
                    shapeIter = tempShapeIter;
                }
            }
            // and now the pig -- it's somewhere in the middle of the poly:
            else if (shape.tags & ShapeRef::SHAPE_INTERNAL_EDGE) {
                std::vector<int> testnodes;
                size_t j;
                for (j = 0; j < static_cast<size_t>(shape.polyrefs.size()); j++) {
                    genlib::addIfNotExists(testnodes, static_cast<int>(shape.polyrefs[j]));
                }
                PixelRef pix2 = pixelate(p);
                pix2.move(PixelRef::NEGVERTICAL); // move pix2 down, search for this shape...
                const std::vector<ShapeRef> *pixelShapes =
                    &m_pixelShapes(static_cast<size_t>(pix2.y), static_cast<size_t>(pix2.x));
                // bit of code duplication like this, but easier on params to this
                // function:
                auto iter = std::find(pixelShapes->begin(), pixelShapes->end(), shape.shapeRef);
                while (iter != pixelShapes->end()) {
                    for (size_t k = 0; k < iter->polyrefs.size(); k++) {
                        genlib::addIfNotExists(testnodes, static_cast<int>(iter->polyrefs[k]));
                    }
                    pix2.move(PixelRef::NEGVERTICAL); // move pix2 down, search for this
                                                      // shape...
                    if (includes(pix2)) {
                        pixelShapes = &m_pixelShapes(static_cast<size_t>(pix2.y),
                                                     static_cast<size_t>(pix2.x));
                        iter = std::find(pixelShapes->begin(), pixelShapes->end(), shape.shapeRef);
                    } else {
                        iter = pixelShapes->end();
                    }
                }
                int alpha2 = 0;
                int counter2 = 0;
                int parity2 = -1;

                for (j = 0; j < testnodes.size(); j++) {
                    Line4f lineb = Line4f(
                        poly.points[static_cast<size_t>(testnodes[j])],
                        poly.points[(static_cast<size_t>(testnodes[j] + 1) % poly.points.size())]);
                    if (lineb.bottomLeft.x <= p.x && lineb.topRight.x >= p.x) {
                        // crosses or touches... but we need to check
                        // touching exception:
                        if (lineb.topRight.x == p.x) {
                            if (parity2 * lineb.by() >= parity2 * p.y) {
                                alpha2 -= 1;
                                counter2++;
                            }
                        }
                        // the other touching exception
                        else if (lineb.bottomLeft.x == p.x) {
                            if (parity2 * lineb.ay() >= parity2 * p.y) {
                                alpha2 += 1;
                                // n.b., no counter here
                            }
                        }
                        // at this stage we know the line isn't vertical, so we can find the
                        // intersection point:
                        else if (parity2 * (lineb.grad(LineAxis::YAXIS) * (p.x - lineb.ax()) +
                                            lineb.ay()) >=
                                 parity2 * p.y) {
                            counter2++;
                        }
                    }
                }
                if (counter2 % 2 != 0 && alpha2 == 0) {
                    shapeIter = m_shapes.find(static_cast<int>(shape.shapeRef));
                }
            }
        }
    }
    return (shapeIter == m_shapes.end())
               ? std::optional<size_t>(std::nullopt)
               : std::distance(m_shapes.begin(), shapeIter); // note convert to -1
}

// also note that you may want to find a close poly line or point
// if you can't find a point in poly (or even if you can)

// (see also getClosestVertex below)

// returns a rowid *not* a shape key

int ShapeMap::getClosestOpenGeom(const Point2f &p) const {
    if (!m_region.contains(p)) {
        return -1;
    }

    PixelRef pix = pixelate(p);

    auto shapeIter = m_shapes.end();
    double mindist = -1;
    const std::vector<ShapeRef> &shapeRefs =
        m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    for (const ShapeRef &ref : shapeRefs) {
        if (ref.tags & ShapeRef::SHAPE_OPEN) {
            double thisdist = -1.0;
            auto tempShapeIter = m_shapes.find(static_cast<int>(ref.shapeRef));
            if (tempShapeIter == m_shapes.end()) {
                throw genlib::RuntimeException("Shape " + std::to_string(ref.shapeRef) +
                                               " not found when getting the closest open geometry");
            }
            const SalaShape &poly = tempShapeIter->second;
            switch (poly.m_type) {
            case SalaShape::SHAPE_POINT:
                thisdist = p.dist(poly.m_centroid);
                break;
            case SalaShape::SHAPE_LINE:
                thisdist = poly.m_region.dist(p); // note, in this case m_region is a line
                break;
            case SalaShape::SHAPE_POLY:
            case SalaShape::SHAPE_POLY | SalaShape::SHAPE_CCW: // note CCW should never have
                                                               // happened, but it has
                for (size_t j = 0; j < ref.polyrefs.size(); j++) {
                    Line4f line(poly.points[static_cast<size_t>(ref.polyrefs[j])],
                                poly.points[static_cast<size_t>(ref.polyrefs[j] + 1)]);
                    double tempthisdist = line.dist(p);
                    if (tempthisdist != -1 && (thisdist == -1 || tempthisdist < thisdist)) {
                        thisdist = tempthisdist;
                    }
                }
                break;
            }
            if (thisdist != -1.0 && (mindist == -1 || thisdist < mindist)) {
                mindist = thisdist;
                shapeIter = tempShapeIter;
            }
        }
    }

    return (shapeIter == m_shapes.end())
               ? -1
               : static_cast<int>(std::distance(m_shapes.begin(),
                                                shapeIter)); // note conversion to -1
}

Point2f ShapeMap::getClosestVertex(const Point2f &p) const {
    Point2f vertex; // null by default

    if (!m_region.contains(p)) {
        return vertex; // will be null in this case
    }

    PixelRef pix = pixelate(p);

    double mindist = -1.0;
    const std::vector<ShapeRef> &shapeRefs =
        m_pixelShapes(static_cast<size_t>(pix.y), static_cast<size_t>(pix.x));
    for (const ShapeRef &ref : shapeRefs) {
        double thisdist = -1.0;
        Point2f thisvertex;
        auto shapeIter = m_shapes.find(static_cast<int>(ref.shapeRef));
        if (shapeIter == m_shapes.end()) {
            throw genlib::RuntimeException("Shape " + std::to_string(ref.shapeRef) +
                                           " not found when trying to get the closest vertex");
        }
        const SalaShape &poly = shapeIter->second;
        switch (poly.m_type) {
        case SalaShape::SHAPE_POINT:
            thisvertex = poly.m_centroid;
            thisdist = p.dist(thisvertex);
            break;
        case SalaShape::SHAPE_LINE: {
            double d1 = p.dist(poly.m_region.start());
            double d2 = p.dist(poly.m_region.end());
            if (d1 < d2) {
                thisvertex = poly.m_region.start();
                thisdist = d1;
            } else {
                thisvertex = poly.m_region.end();
                thisdist = d2;
            }
        } break;
        default: // either a poly line or a polygon
            for (size_t j = 0; j < ref.polyrefs.size(); j++) {
                double d1 = p.dist(poly.points[static_cast<size_t>(ref.polyrefs[j])]);
                // note this can be used for both open / closed with the % poly.size()
                double d2 = p.dist(
                    poly.points[static_cast<size_t>(ref.polyrefs[j] + 1) % poly.points.size()]);
                if (thisdist == -1 || d1 < thisdist) {
                    thisvertex = poly.points[static_cast<size_t>(ref.polyrefs[j])];
                    thisdist = d1;
                }
                if (d2 < thisdist) {
                    thisvertex =
                        poly.points[static_cast<size_t>(ref.polyrefs[j] + 1) % poly.points.size()];
                    thisdist = d2;
                }
            }
            break;
        }
        if (thisdist != -1.0 && (mindist == -1.0 || thisdist < mindist)) {
            mindist = thisdist;
            vertex = thisvertex;
        }
    }

    return vertex;
}

// code to add intersections when shapes are added to the graph one by one:
size_t ShapeMap::connectIntersected(size_t rowid, bool linegraph) {
    auto shaperefIter = genlib::getMapAtIndex(m_shapes, rowid);
    auto connCol = m_attributes->getOrInsertLockedColumn("Connectivity");
    size_t lengCol = 0;
    if (linegraph) {
        // historically line length has always been added at this point
        lengCol = m_attributes->getOrInsertLockedColumn("Line Length");
    }
    // all indices should match... this grows connectors if necessary to same
    // length as shapes
    while (m_connectors.size() < m_shapes.size()) {
        m_connectors.push_back(Connector());
    }
    m_connectors[rowid].connections =
        linegraph
            ? getLineConnections(shaperefIter->first,
                                 TOLERANCE_B * std::max(m_region.height(), m_region.width()))
            : getShapeConnections(shaperefIter->first,
                                  TOLERANCE_B * std::max(m_region.height(), m_region.width()));

    auto &row = getAttributeRowFromShapeIndex(rowid);
    row.setValue(connCol, static_cast<float>(m_connectors[rowid].connections.size()));
    if (linegraph) {
        row.setValue(lengCol, static_cast<float>(shaperefIter->second.getLength()));
    }
    // now go through our connections, and add ourself:
    const auto &connections = m_connectors[rowid].connections;
    for (auto connection : connections) {
        if (connection != rowid) { // <- exclude self!
            genlib::insert_sorted(m_connectors[connection].connections, rowid);
            auto &connectionRow = getAttributeRowFromShapeIndex(connection);
            connectionRow.incrValue(connCol);
        }
    }
    return m_connectors[rowid].connections.size();
}

// this assumes this is a line map (to speed up axial map creation)
// use the other version, getShapeConnections for arbitrary shape-shape
// connections note, connections are listed by rowid in list, *not* reference
// number (so they may vary: must be checked carefully when shapes are removed /
// added)
std::vector<size_t> ShapeMap::getLineConnections(int lineref, double tolerance) {
    std::vector<size_t> connections;

    auto shapeIter = m_shapes.find(lineref);
    if (shapeIter == m_shapes.end()) {
        throw genlib::RuntimeException("Shape " + std::to_string(lineref) +
                                       " not found when getting line connections");
    }
    SalaShape &poly = shapeIter->second;
    if (!poly.isLine()) {
        return std::vector<size_t>();
    }
    const Line4f &l = poly.getLine();

    std::unordered_set<ShapeRef, ShapeRefHash> shapesToTest;

    // As of version 10, self-connections are *not* added
    // In the past:
    // <exclude> it's useful to have yourself in your connections list
    // (apparently! -- this needs checking, as most of the time it is then checked
    // to exclude self again!) </exclude> <exclude>
    // connections.add(m_shapes.searchindex(lineref)); </exclude>

    shapesToTest.insert(static_cast<unsigned int>(lineref));

    PixelRefVector list = pixelateLine(l);

    for (size_t i = 0; i < list.size(); i++) {
        const std::vector<ShapeRef> &shapeRefs =
            m_pixelShapes(static_cast<size_t>(list[i].y), static_cast<size_t>(list[i].x));
        for (const ShapeRef &shape : shapeRefs) {
            shapesToTest.insert(shape);
        }
    }
    for (const ShapeRef &shape : shapesToTest) {
        if ((shape.tags & ShapeRef::SHAPE_OPEN) == ShapeRef::SHAPE_OPEN) {
            auto shapeIter1 = m_shapes.find(static_cast<int>(shape.shapeRef));
            if (shapeIter1 == m_shapes.end()) {
                throw genlib::RuntimeException("Shape " + std::to_string(shape.shapeRef) +
                                               " not found while testing line connections");
            }
            const Line4f &line = shapeIter1->second.getLine();
            if (line.Region4f::intersects(l, line.length() * tolerance)) {
                // n.b. originally this followed the logic that we must normalise
                // intersect_line properly: tolerance * line length one * line length
                // two in fact, works better if it's just line.length() * tolerance...
                if (line.Line4f::intersects(l, line.length() * tolerance)) {
                    auto shapeIdx =
                        genlib::findIndexFromKey(m_shapes, static_cast<int>(shape.shapeRef));
                    genlib::insert_sorted(connections, static_cast<size_t>(shapeIdx));
                }
            }
        }
    }

    return connections;
}

// this is only problematic as there is lots of legacy code with shape-in-shape
// testing,
std::vector<size_t> ShapeMap::getShapeConnections(int shaperef, double tolerance) const {
    // In versions prior to 10, note that unlike getLineConnections,
    // self-connection is excluded by all of the following functions As of version
    // 10, both getShapeConnections and getLineConnections exclude self-connection

    std::vector<size_t> connections;

    auto shapeIter = m_shapes.find(shaperef);
    if (shapeIter != m_shapes.end()) {
        const SalaShape &shape = shapeIter->second;
        if (shape.isPoint()) {
            // a point is simple, it never intersects itself:
            connections = pointInPolyList(shape.getPoint());
        } else if (shape.isPolygon()) {
            // a closed poly is actually quite simple too as we already have code
            // using a polyref:
            connections = polyInPolyList(shaperef, tolerance);
        } else if (shape.isLine()) {
            // line is a bit slow because there's no tested shape as in
            // getLineConnections, but similar:
            connections = lineInPolyList(shape.getLine(), shaperef, tolerance);
        } else if (shape.isPolyLine()) {
            // this is the worst for efficiency: potential for many possible retries
            // of the same shape:
            for (size_t i = 1; i < shape.points.size() - 1; i++) {
                Line4f li(shape.points[i - 1], shape.points[i]);
                connections = lineInPolyList(li, shaperef, tolerance);
            }
        }
    }

    return connections;
}

// for any geometry, not just line to lines
void ShapeMap::makeShapeConnections() {
    if (m_hasgraph) {
        m_connectors.clear();
        m_attributes->clear();
        m_links.clear();
        m_unlinks.clear();

        // note, expects these to be numbered 0, 1...
        int connCol = static_cast<int>(m_attributes->insertOrResetLockedColumn("Connectivity"));

        int i = -1;
        for (const auto &shape : m_shapes) {
            i++;
            int key = shape.first;
            auto &row = m_attributes->addRow(AttributeKey(key));
            // all indices should match...
            m_connectors.push_back(Connector());
            m_connectors[static_cast<size_t>(i)].connections = getShapeConnections(
                key, TOLERANCE_B * std::max(m_region.height(), m_region.width()));
            row.setValue(
                static_cast<size_t>(connCol),
                static_cast<float>(m_connectors[static_cast<size_t>(i)].connections.size()));
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////

// note: uses rowid not key
double ShapeMap::getLocationValue(const Point2f &point, std::optional<size_t> attributeIdx) const {
    double val = -1.0;
    int x = pointInPoly(point);
    if (x == -1) {
        // try looking for a polyline instead
        x = getClosestOpenGeom(point);
    }
    if (x != -1) {
        int key = getShapeRefFromIndex(static_cast<size_t>(x))->first;
        if (!attributeIdx.has_value()) {
            val = static_cast<float>(key);
        } else {
            auto &row = m_attributes->getRow(AttributeKey(key));
            val = row.getValue(attributeIdx.value());
        }
    }
    return (x == -1) ? -2.0 : val; // -2.0 is returned when point cannot be
                                   // associated with a poly
}

const std::map<int, SalaShape> ShapeMap::getShapesInRegion(const Region4f &r) const {

    std::map<int, SalaShape> shapesInRegion;

    if (r.bottomLeft == r.topRight) {
        // note: uses index not key
        int index = pointInPoly(r.bottomLeft);
        if (index == -1) {
            // try looking for a polyline instead
            index = getClosestOpenGeom(r.bottomLeft);
        }
        if (index != -1) {
            shapesInRegion.insert(*getShapeRefFromIndex(static_cast<size_t>(index)));
        }
    } else {
        PixelRef bl = pixelate(r.bottomLeft);
        PixelRef tr = pixelate(r.topRight);
        for (int i = bl.x; i <= tr.x; i++) {
            for (int j = bl.y; j <= tr.y; j++) {
                const std::vector<ShapeRef> &shapeRefs =
                    m_pixelShapes(static_cast<size_t>(j), static_cast<size_t>(i));
                for (const ShapeRef &shapeRef : shapeRefs) {
                    // relies on indices of shapes and attributes being aligned
                    auto shape = m_shapes.find(static_cast<int>(shapeRef.shapeRef));
                    if (shape != m_shapes.end()) {
                        shapesInRegion.insert(*shape);
                    }
                }
            }
        }
    }

    return shapesInRegion;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::readNameType(std::istream &stream) {
    m_mapType = ShapeMap::EMPTYMAP;

    // clear old:
    m_shapes.clear();
    m_attributes->clear();
    m_connectors.clear();
    m_links.clear();
    m_unlinks.clear();
    m_undobuffer.clear();

    // name
    m_name = dXstring::readString(stream);

    stream.read(reinterpret_cast<char *>(&m_mapType), sizeof(m_mapType));
    return true;
}

bool ShapeMap::readPart2(std::istream &stream) {
    // PixelBase read
    // read extents:
    stream.read(reinterpret_cast<char *>(&m_region), sizeof(m_region));
    // read rows / cols
    int rows, cols;
    stream.read(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.read(reinterpret_cast<char *>(&cols), sizeof(cols));
    m_rows = static_cast<size_t>(rows);
    m_cols = static_cast<size_t>(cols);
    // calculate geom data:
    m_tolerance = std::max(m_region.width(), m_region.height()) * TOLERANCE_A;

    // read next object ref to be used:
    stream.read(reinterpret_cast<char *>(&m_objRef), sizeof(m_objRef));
    int deprInt;
    stream.read(reinterpret_cast<char *>(&deprInt), sizeof(deprInt));

    // read shape data
    int count = 0;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (int j = 0; j < count; j++) {
        int key;
        stream.read(reinterpret_cast<char *>(&key), sizeof(key));
        auto iter = m_shapes.insert(std::make_pair(key, SalaShape())).first;
        iter->second.read(stream);
    }

    // read object data (currently unused)
    // PK: As the above comment (and others regarding the m_objects
    // functionality) suggest, these are no longer used so they can
    // just be skipped if ever found
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (int k = 0; k < count; k++) {
        int key;
        stream.read(reinterpret_cast<char *>(&key), sizeof(key));
        unsigned int size;
        stream.read(reinterpret_cast<char *>(&size), sizeof(size));
        stream.ignore(static_cast<long>(sizeof(int) * static_cast<size_t>(std::streamsize(size))));
    }
    // read attribute data
    m_attributes->read(stream, m_layers);

    return true;
}

bool ShapeMap::readPart3(std::istream &stream) {

    // prepare pixel map:
    m_pixelShapes = genlib::ColumnMatrix<std::vector<ShapeRef>>(m_rows, m_cols);
    // Now add the pixel shapes pixel map:
    // pixelate all polys in the pixel structure:
    for (const auto &shape : m_shapes) {
        makePolyPixels(shape.first);
    }

    // shape connections:
    int count = 0;
    stream.read(reinterpret_cast<char *>(&count), sizeof(count));
    for (int i = 0; i < count; i++) {
        m_connectors.push_back(Connector());
        m_connectors[static_cast<size_t>(i)].read(stream);
    }
    dXreadwrite::readFromCastIntoVector<OrderedIntPair>(stream, m_links);
    dXreadwrite::readFromCastIntoVector<OrderedIntPair>(stream, m_unlinks);

    // some miscellaneous extra data for mapinfo files
    m_hasMapInfoData = false;
    char x = static_cast<char>(stream.get());
    if (x == 'm') {
        m_mapinfodata = MapInfoData();
        m_mapinfodata.read(stream);
        m_hasMapInfoData = true;
    }
    return true;
}

std::tuple<bool, bool, bool, int> ShapeMap::read(std::istream &stream) {

    bool read = readNameType(stream);

    bool show = true; // <- by default show
    // turn off selection / editable etc
    bool editable = false;

    stream.read(reinterpret_cast<char *>(&show), sizeof(show));
    stream.read(reinterpret_cast<char *>(&editable), sizeof(editable));

    read = read && readPart2(stream);

    // sala does not handle display information any more.
    // re-create this function with the above and below parts
    // handling this int variable in order to get a Depthmap-like
    // experience (where the displayed attribute is in the file)
    int displayedAttribute;
    stream.read(reinterpret_cast<char *>(&displayedAttribute), sizeof(displayedAttribute));

    read = read && readPart3(stream);

    return std::tie(read, editable, show, displayedAttribute);
}

bool ShapeMap::writeNameType(std::ostream &stream) const {
    // name
    dXstring::writeString(stream, m_name);

    stream.write(reinterpret_cast<const char *>(&m_mapType), sizeof(m_mapType));
    return true;
}

bool ShapeMap::writePart2(std::ostream &stream) const {
    // PixelBase write
    // write extents:
    stream.write(reinterpret_cast<const char *>(&m_region), sizeof(m_region));
    // write rows / cols
    int rows = static_cast<int>(m_rows);
    int cols = static_cast<int>(m_cols);
    stream.write(reinterpret_cast<char *>(&rows), sizeof(rows));
    stream.write(reinterpret_cast<char *>(&cols), sizeof(cols));

    // write next object ref to be used:
    stream.write(reinterpret_cast<const char *>(&m_objRef), sizeof(m_objRef));

    // left here for backwards-compatibility
    // TODO: Remove at next iteration of the .graph file format
    int largestShapeRef = m_shapes.empty() ? -1 : m_shapes.rbegin()->first;
    stream.write(reinterpret_cast<const char *>(&largestShapeRef), sizeof(largestShapeRef));

    // write shape data
    int count = static_cast<int>(m_shapes.size());
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));
    for (const auto &shape : m_shapes) {
        int key = shape.first;
        stream.write(reinterpret_cast<const char *>(&key), sizeof(key));
        shape.second.write(stream);
    }
    // write object data (currently unused)
    count = 0;
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));

    // write attribute data
    m_attributes->write(stream, m_layers);
    return true;
}

bool ShapeMap::writePart3(std::ostream &stream) const {

    // write connections data
    int count = static_cast<int>(m_connectors.size());
    stream.write(reinterpret_cast<const char *>(&count), sizeof(count));

    for (size_t i = 0; i < m_connectors.size(); i++) {
        m_connectors[i].write(stream);
    }

    dXreadwrite::writeCastVector<OrderedIntPair>(stream, m_links);
    dXreadwrite::writeCastVector<OrderedIntPair>(stream, m_unlinks);

    // some miscellaneous extra data for mapinfo files
    if (m_hasMapInfoData) {
        stream.put('m');
        m_mapinfodata.write(stream);
    } else {
        stream.put('x');
    }
    return true;
}

bool ShapeMap::write(std::ostream &stream, const std::tuple<bool, bool, int> &displayData) const {

    bool written = writeNameType(stream);

    auto [editable, show, displayedAttribute] = displayData;

    stream.write(reinterpret_cast<const char *>(&show), sizeof(show));
    stream.write(reinterpret_cast<const char *>(&editable), sizeof(editable));

    written = written && writePart2(stream);

    stream.write(reinterpret_cast<const char *>(&displayedAttribute), sizeof(displayedAttribute));

    written = written && writePart3(stream);
    return written;
}

bool ShapeMap::output(std::ofstream &stream, char delimiter) {
    auto const streamFlags = stream.flags();
    stream << "Ref";
    if ((m_mapType & LINEMAP) == 0) {
        stream << delimiter << "cx" << delimiter << "cy";
    } else {
        stream << delimiter << "x1" << delimiter << "y1" << delimiter << "x2" << delimiter << "y2";
    }

    // TODO: For compatibility write the columns in alphabetical order
    // but the physical columns in the order inserted

    std::vector<size_t> indices(m_attributes->getNumColumns());
    std::iota(indices.begin(), indices.end(), static_cast<size_t>(0));

    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return m_attributes->getColumnName(a) < m_attributes->getColumnName(b);
    });
    for (size_t idx : indices) {
        stream << delimiter << m_attributes->getColumnName(idx);
    }

    stream << std::endl;

    for (auto iter = m_attributes->begin(); iter != m_attributes->end(); iter++) {
        int key = iter->getKey().value;
        if (isObjectVisible(m_layers, iter->getRow())) {
            stream << key;
            const auto &shape = m_shapes[key];
            if ((m_mapType & LINEMAP) == 0) {
                stream << delimiter << shape.m_centroid.x << delimiter << shape.m_centroid.y;
            } else {
                stream.precision(12); // TODO: Here for compatibility with old version
                const Line4f &li = shape.getLine();
                stream << delimiter << li.start().x << delimiter << li.start().y << delimiter
                       << li.end().x << delimiter << li.end().y;
            }
            stream.precision(8); // TODO: Here for compatibility with old version
            for (size_t idx : indices) {
                stream << delimiter << iter->getRow().getValue(idx);
            }
            stream << std::endl;
        }
    }
    stream.flags(streamFlags);
    return true;
}

bool ShapeMap::importPoints(const std::vector<Point2f> &points, const sala::Table &data) {
    // assumes that points and data come in the same order

    std::vector<int> shapeRefs;

    for (auto &point : points) {
        shapeRefs.push_back(makePointShape(point));
    }

    return importData(data, std::move(shapeRefs));
}

bool ShapeMap::importPointsWithRefs(const std::map<int, Point2f> &points, const sala::Table &data) {
    // assumes that points and data come in the same order

    std::vector<int> shapeRefs;

    for (auto &point : points) {
        shapeRefs.push_back(makePointShapeWithRef(point.second, point.first));
    }

    return importData(data, std::move(shapeRefs));
}

bool ShapeMap::importLines(const std::vector<Line4f> &lines, const sala::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shapeRefs;

    for (auto &line : lines) {
        shapeRefs.push_back(makeLineShape(line));
    }

    return importData(data, std::move(shapeRefs));
}

bool ShapeMap::importLinesWithRefs(const std::map<int, Line4f> &lines, const sala::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shapeRefs;

    for (auto &line : lines) {
        shapeRefs.push_back(makeLineShapeWithRef(line.second, line.first));
    }

    return importData(data, std::move(shapeRefs));
}

bool ShapeMap::importPolylines(const std::vector<sala::Polyline> &polylines,
                               const sala::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shapeRefs;

    for (auto &polyline : polylines) {
        shapeRefs.push_back(makePolyShape(polyline.vertices, !polyline.closed));
    }

    return importData(data, std::move(shapeRefs));
}

bool ShapeMap::importPolylinesWithRefs(const std::map<int, sala::Polyline> &polylines,
                                       const sala::Table &data) {
    // assumes that lines and data come in the same order

    std::vector<int> shapeRefs;

    for (auto &polyline : polylines) {
        shapeRefs.push_back(makePolyShapeWithRef(polyline.second.vertices, !polyline.second.closed,
                                                 polyline.first));
    }

    return importData(data, std::move(shapeRefs));
}

bool ShapeMap::importData(const sala::Table &data, std::vector<int> shapeRefs) {
    for (auto &column : data) {
        std::string colName = column.first;
        std::replace(colName.begin(), colName.end(), '_', ' ');
        dXstring::makeInitCaps(colName);

        if (colName.empty())
            continue;

        int colIndex = static_cast<int>(m_attributes->insertOrResetColumn(colName));

        if (colIndex == -1) {
            // error adding column (e.g., duplicate column names)
            continue;
        }

        std::unordered_map<std::string, size_t> colcodes;

        for (size_t i = 0; i < column.second.size(); i++) {
            std::string cellValue = column.second[i];
            double value = 0;
            if (dXstring::isDouble(cellValue)) {
                value = stod(cellValue);
            } else {
                std::unordered_map<std::string, size_t>::iterator cellAt = colcodes.find(cellValue);
                if (cellAt == colcodes.end()) {

                    // TODO:
                    // It seems that the original intention here was that if we are past
                    // 32 unique values, we should stop trying to make the column
                    // categorical and fill the rest of the values with -1.0f. It's not
                    // possible to test the original implementation because the app
                    // crashes if we load a file with more than 32 unique values. When and
                    // if we have a robust implementation of an attribute table that
                    // allows for both categorical and plain string attributes this should
                    // be re-examined for a better way to classify the column as either.
                    // Meanwhile after this threshold (32) we set the whole column to -1
                    // so that it does not give the impression it worked when it's
                    // actually half-baked

                    if (colcodes.size() >= 32) {
                        for (size_t j = 0; j < column.second.size(); j++) {
                            m_attributes->getRow(AttributeKey(shapeRefs[j]))
                                .setValue(static_cast<size_t>(colIndex), -1.0f);
                        }
                        continue;
                    } else {
                        value = static_cast<double>(colcodes.size());
                        colcodes.insert(std::make_pair(cellValue, colcodes.size()));
                    }
                } else {
                    value = static_cast<double>(cellAt->second);
                }
            }
            m_attributes->getRow(AttributeKey(shapeRefs[i]))
                .setValue(static_cast<size_t>(colIndex), static_cast<float>(value));
        }
    }
    return true;
}

// copied from SpacePixel

PixelRef ShapeMap::pixelate(const Point2f &p, bool constrain, int) const {
    PixelRef r;

    Point2f p1 = p;
    p1.normalScale(m_region.bottomLeft, m_region.width(), m_region.height());

    if (constrain) {
        if (p1.x <= 0.0) {
            r.x = 0;
        } else if (p1.x >= 1.0) {
            r.x = static_cast<short>(m_cols - 1);
        } else {
            r.x = static_cast<short>(floor(p1.x * static_cast<double>(m_cols)));
        }
    } else {
        r.x = static_cast<short>(floor(p1.x * static_cast<double>(m_cols)));
    }

    if (constrain) {
        if (p1.y <= 0.0) {
            r.y = 0;
        } else if (p1.y >= 1.0) {
            r.y = static_cast<short>(m_rows - 1);
        } else {
            r.y = static_cast<short>(floor(p1.y * static_cast<double>(m_rows)));
        }
    } else {
        r.y = static_cast<short>(floor(p1.y * static_cast<double>(m_rows)));
    }

    return r;
}

void ShapeMap::copyMapInfoBaseData(const ShapeMap &sourceMap) {
    m_mapinfodata = MapInfoData();
    m_mapinfodata.m_coordsys = sourceMap.getMapInfoData().m_coordsys;
    m_mapinfodata.m_bounds = sourceMap.getMapInfoData().m_bounds;
    m_hasMapInfoData = true;
}

///////////////////////////////////////////////////////////////////////////////////

int ShapeMap::loadMifMap(std::istream &miffile, std::istream &midfile) {
    m_mapinfodata = MapInfoData();
    int retvar = m_mapinfodata.import(miffile, midfile, *this);
    if (retvar == MINFO_OK)
        m_hasMapInfoData = true;
    return retvar;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool ShapeMap::outputMifMap(std::ostream &miffile, std::ostream &midfile) {
    if (!m_hasMapInfoData) {
        MapInfoData mapinfodata;
        mapinfodata.exportFile(miffile, midfile, *this);
    } else {
        m_mapinfodata.exportFile(miffile, midfile, *this);
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////

// Code for explicit linking / unlinking

bool ShapeMap::linkShapes(const Point2f &p, PixelRef p2) {
    auto index1 = std::distance(m_shapes.begin(), m_shapes.find(p2));
    // note: uses rowid not key
    int index2 = pointInPoly(p);
    if (index2 == -1) {
        // try looking for a polyline instead
        index2 = getClosestOpenGeom(p);
    }
    if (index2 == -1) {
        return false;
    }

    linkShapes(static_cast<size_t>(index1), static_cast<size_t>(index2));

    return true;
}

bool ShapeMap::linkShapesFromRefs(int ref1, int ref2) {
    auto index1 = genlib::findIndexFromKey(m_shapes, ref1);
    if (index1 < 0) {
        throw new genlib::RuntimeException("Shape reference " + std::to_string(ref1) +
                                           " not found to link shapes");
    }
    auto index2 = genlib::findIndexFromKey(m_shapes, ref2);
    if (index2 < 0) {
        throw new genlib::RuntimeException("Shape reference " + std::to_string(ref2) +
                                           " not found to link shapes");
    }
    return linkShapes(static_cast<size_t>(index1), static_cast<size_t>(index2));
}

bool ShapeMap::linkShapes(size_t index1, size_t index2) {
    auto connCol = m_attributes->getOrInsertLockedColumn("Connectivity");
    bool update = false;

    if (index1 != index2) {
        // link these lines...
        // first look for explicit unlinks and clear
        OrderedSizeTPair link(index1, index2);
        auto unlinkiter = std::find(m_unlinks.begin(), m_unlinks.end(), link);
        if (unlinkiter != m_unlinks.end()) {
            m_unlinks.erase(unlinkiter);
            update = true;
        } else {
            // then check not linked already
            auto &connections1 = m_connectors[index1].connections;
            auto &connections2 = m_connectors[index2].connections;
            auto linkIter1 = std::find(connections1.begin(), connections1.end(), index2);
            auto linkIter2 = std::find(connections2.begin(), connections2.end(), index1);
            if (linkIter1 == connections1.end() && linkIter2 == connections2.end()) {
                // finally, link the two lines
                genlib::addIfNotExists(m_links, link);
                update = true;
            }
        }
    }

    if (update) {
        genlib::insert_sorted(m_connectors[index1].connections, index2);
        genlib::insert_sorted(m_connectors[index2].connections, index1);
        auto &row1 = getAttributeRowFromShapeIndex(index1);
        auto &row2 = getAttributeRowFromShapeIndex(index2);
        row1.incrValue(connCol);
        row2.incrValue(connCol);
    }

    return update;
}

// this version is used to link segments in segment analysis
// note it only links one way!
bool ShapeMap::linkShapes(size_t id1, int dir1, size_t id2, int dir2, float weight) {
    bool success = false;
    Connector &connector = m_connectors[static_cast<size_t>(id1)];
    if (dir1 == 1) {
        success = genlib::addIfNotExists(
            connector.forwardSegconns,
            SegmentRef(static_cast<int8_t>(dir2), static_cast<int8_t>(id2)), weight);
    } else {
        success = genlib::addIfNotExists(
            connector.backSegconns, SegmentRef(static_cast<int8_t>(dir2), static_cast<int8_t>(id2)),
            weight);
    }

    // checking success != -1 avoids duplicate entries adding to connectivity
    if (success) {
        auto connCol = m_attributes->getOrInsertLockedColumn("Connectivity");
        auto &row = getAttributeRowFromShapeIndex(id1);
        row.incrValue(connCol);
        auto weightCol = m_attributes->getOrInsertLockedColumn("Weighted Connectivity");
        row.incrValue(weightCol, weight);
    }

    return true;
}

bool ShapeMap::unlinkShapes(const Point2f &p, PixelRef p2) {
    auto index1 = std::distance(m_shapes.begin(), m_shapes.find(p2));
    int index2 = pointInPoly(p);
    if (index2 == -1) {
        // try looking for a polyline instead
        index2 = getClosestOpenGeom(p);
    }
    if (index2 == -1) {
        return false;
    }

    unlinkShapes(static_cast<size_t>(index1), static_cast<size_t>(index2));

    return true;
}

bool ShapeMap::unlinkShapes(const Point2f &p1, const Point2f &p2) {
    int index1 = pointInPoly(p1);
    if (index1 == -1) {
        // try looking for a polyline instead
        index1 = getClosestOpenGeom(p1);
    }
    if (index1 == -1) {
        return false;
    }
    int index2 = pointInPoly(p2);
    if (index2 == -1) {
        // try looking for a polyline instead
        index2 = getClosestOpenGeom(p2);
    }
    if (index2 == -1) {
        return false;
    }

    unlinkShapes(static_cast<size_t>(index1), static_cast<size_t>(index2));

    return true;
}

bool ShapeMap::unlinkShapesFromRefs(int ref1, int ref2) {
    auto index1 = genlib::findIndexFromKey(m_shapes, ref1);
    if (index1 < 0) {
        throw new genlib::RuntimeException("Shape reference " + std::to_string(ref1) +
                                           " not found to unlink shapes");
    }
    auto index2 = genlib::findIndexFromKey(m_shapes, ref2);
    if (index2 < 0) {
        throw new genlib::RuntimeException("Shape reference " + std::to_string(ref2) +
                                           " not found to unlink shapes");
    }
    return unlinkShapes(static_cast<size_t>(index1), static_cast<size_t>(index2));
}

// note: uses rowids rather than shape key
bool ShapeMap::unlinkShapes(size_t index1, size_t index2) {
    auto connCol = m_attributes->getColumnIndex("Connectivity");
    bool update = false;

    if (index1 != index2) {
        // unlink these shapes...
        // first look for explicit links and clear
        OrderedSizeTPair unlink(index1, index2);
        auto linkiter = std::find(m_links.begin(), m_links.end(), unlink);
        if (linkiter != m_links.end()) {
            m_links.erase(linkiter);
            update = true;
        } else {
            // then check if linked already
            auto &connections1 = m_connectors[static_cast<size_t>(index1)].connections;
            auto &connections2 = m_connectors[static_cast<size_t>(index2)].connections;
            auto linkIter1 = std::find(connections1.begin(), connections1.end(), index2);
            auto linkIter2 = std::find(connections2.begin(), connections2.end(), index1);
            if (linkIter1 != connections1.end() && linkIter2 != connections2.end()) {
                // finally, unlink the two shapes
                genlib::addIfNotExists(m_unlinks, unlink);
                update = true;
            }
        }
    }

    if (update) {
        genlib::findAndErase(m_connectors[static_cast<size_t>(index1)].connections, index2);
        genlib::findAndErase(m_connectors[static_cast<size_t>(index2)].connections, index1);
        auto &row1 = getAttributeRowFromShapeIndex(index1);
        auto &row2 = getAttributeRowFromShapeIndex(index2);
        row1.incrValue(connCol, -1.0f);
        row2.incrValue(connCol, -1.0f);
    }
    return update;
}

bool ShapeMap::unlinkShapesByKey(int key1, int key2) {
    auto connCol = m_attributes->getColumnIndex("Connectivity");
    bool update = false;

    auto index1 = static_cast<size_t>(std::distance(m_shapes.begin(), m_shapes.find(key1)));
    auto index2 = static_cast<size_t>(std::distance(m_shapes.begin(), m_shapes.find(key2)));

    if (key1 != key2) {
        // unlink these shapes...
        // first look for explicit links and clear
        OrderedSizeTPair unlink(index1, index2);
        auto linkiter = std::find(m_links.begin(), m_links.end(), unlink);
        if (linkiter != m_links.end()) {
            m_links.erase(linkiter);
            update = true;
        } else {
            // then check if linked already
            auto &connections1 = m_connectors[index1].connections;
            auto &connections2 = m_connectors[index2].connections;
            auto linkIter1 = std::find(connections1.begin(), connections1.end(), index2);
            auto linkIter2 = std::find(connections2.begin(), connections2.end(), index1);
            if (linkIter1 != connections1.end() && linkIter2 != connections2.end()) {
                // finally, unlink the two shapes
                genlib::addIfNotExists(m_unlinks, unlink);
                update = true;
            }
        }
    }

    if (update) {
        genlib::findAndErase(m_connectors[index1].connections, index2);
        genlib::findAndErase(m_connectors[index2].connections, index1);
        auto &row1 = m_attributes->getRow(AttributeKey(key1));
        auto &row2 = m_attributes->getRow(AttributeKey(key1));
        row1.incrValue(connCol, -1.0f);
        row2.incrValue(connCol, -1.0f);
    }
    return update;
}

bool ShapeMap::clearLinks() {
    for (size_t i = 0; i < m_unlinks.size(); i++) {
        const OrderedSizeTPair &link = m_unlinks[i];
        genlib::insert_sorted(m_connectors[link.a].connections, link.b);
        genlib::insert_sorted(m_connectors[link.b].connections, link.a);
    }
    m_unlinks.clear();

    for (size_t j = 0; j < m_links.size(); j++) {
        const OrderedSizeTPair &link = m_links[j];
        genlib::findAndErase(m_connectors[link.a].connections, link.b);
        genlib::findAndErase(m_connectors[link.b].connections, link.a);
    }
    m_links.clear();

    return true;
}

bool ShapeMap::unlinkShapeSet(std::istream &idset, int refcol) {
    std::string line;
    std::vector<std::pair<int, int>> unlinks;
    do {
        std::pair<int, int> unlink;
        dXstring::safeGetline(idset, line);
        if (!line.empty()) {
            auto tokens = dXstring::split(line, '\t');
            if (tokens.size() < 2) {
                return false;
            }
            try {
                unlink.first = stoi(tokens[0]);
                unlink.second = stoi(tokens[1]);
                unlinks.push_back(unlink);
            } catch (const std::invalid_argument &) {
            } catch (const std::out_of_range &) {
            } // don't do anything if it can't parse the numbers, just ignore (e.g.,
              // first line)
        }
    } while (!idset.eof());

    if (refcol != -1) {
        // not using the standard "Ref", find the proper key
        std::vector<AttributeIndexItem> idx = refcol != -1
                                                  ? makeAttributeIndex(*m_attributes, refcol)
                                                  : std::vector<AttributeIndexItem>();

        AttributeKey dummykey(-1);
        AttributeRowImpl dummyrow(*m_attributes);

        for (size_t i = 0; i < unlinks.size(); i++) {
            auto iter =
                genlib::findBinary(idx, AttributeIndexItem(dummykey, unlinks[i].first, dummyrow));
            unlinks[i].first = (iter == idx.end()) ? -1 : iter->key.value;
            iter =
                genlib::findBinary(idx, AttributeIndexItem(dummykey, unlinks[i].second, dummyrow));
            unlinks[i].second = (iter == idx.end()) ? -1 : iter->key.value;
        }
    }
    for (size_t i = 0; i < unlinks.size(); i++) {
        unlinkShapesByKey(unlinks[i].first, unlinks[i].second);
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////

std::vector<SimpleLine> ShapeMap::getAllLinkLines() {
    std::vector<SimpleLine> linkLines;
    for (size_t i = 0; i < m_links.size(); i++) {
        linkLines.push_back(
            SimpleLine(genlib::getMapAtIndex(m_shapes, m_links[i].a)->second.getCentroid(),
                       genlib::getMapAtIndex(m_shapes, m_links[i].b)->second.getCentroid()));
    }
    return linkLines;
}

// note: these functions would need slight work for arbitrary shape overlaps

std::vector<Point2f> ShapeMap::getAllUnlinkPoints() {
    std::vector<Point2f> unlinkPoints;
    for (size_t i = 0; i < m_unlinks.size(); i++) {
        unlinkPoints.push_back(
            genlib::getMapAtIndex(m_shapes, m_unlinks[i].a)
                ->second.getLine()
                .intersection_point(
                    genlib::getMapAtIndex(m_shapes, m_unlinks[i].b)->second.getLine(),
                    TOLERANCE_A));
    }
    return unlinkPoints;
}

void ShapeMap::outputUnlinkPoints(std::ofstream &stream, char delim) {
    auto const streamFlags = stream.flags();
    stream << "x" << delim << "y" << std::endl;

    stream.precision(12);
    for (size_t i = 0; i < m_unlinks.size(); i++) {
        // note, links are stored directly by rowid, not by key:
        Point2f p =
            genlib::getMapAtIndex(m_shapes, m_unlinks[i].a)
                ->second.getLine()
                .intersection_point(
                    genlib::getMapAtIndex(m_shapes, m_unlinks[i].b)->second.getLine(), TOLERANCE_A);
        stream << p.x << delim << p.y << std::endl;
    }
    stream.flags(streamFlags);
}

std::vector<Line4f> ShapeMap::getAllShapesAsLines() const {
    std::vector<Line4f> lines;
    auto newLines = getAllShapesAsSimpleLines();
    for (const auto &line : newLines) {
        lines.emplace_back(line.start(), line.end());
    }
    return lines;
}

std::vector<SimpleLine> ShapeMap::getAllShapesAsSimpleLines() const {
    std::vector<SimpleLine> lines;
    const std::map<int, SalaShape> &allShapes = getAllShapes();
    for (const auto &refShape : allShapes) {
        const SalaShape &shape = refShape.second;
        if (shape.isLine()) {
            lines.push_back(SimpleLine(shape.getLine()));
        } else if (shape.isPolyLine() || shape.isPolygon()) {
            for (size_t n = 0; n < shape.points.size() - 1; n++) {
                lines.push_back(SimpleLine(shape.points[n], shape.points[n + 1]));
            }
            if (shape.isPolygon()) {
                lines.push_back(SimpleLine(shape.points.back(), shape.points.front()));
            }
        }
    }
    return lines;
}

std::vector<std::pair<SimpleLine, PafColor>>
ShapeMap::getAllSimpleLinesWithColour(const std::set<int> &selSet) {
    std::vector<std::pair<SimpleLine, PafColor>> colouredLines;
    std::map<int, SalaShape> &allShapes = getAllShapes();
    for (auto &refShape : allShapes) {
        SalaShape &shape = refShape.second;
        PafColor colour(dXreimpl::getDisplayColor(
            AttributeKey(refShape.first), m_attributes->getRow(AttributeKey(refShape.first)),
            *m_attribHandle.get(), selSet, true));
        if (shape.isLine()) {
            colouredLines.push_back(
                std::pair<SimpleLine, PafColor>(SimpleLine(shape.getLine()), colour));
        } else if (shape.isPolyLine()) {
            for (size_t n = 0; n < shape.points.size() - 1; n++) {
                colouredLines.push_back(std::pair<SimpleLine, PafColor>(
                    SimpleLine(shape.points[n], shape.points[n + 1]), colour));
            }
        }
    }
    return colouredLines;
}

std::vector<std::pair<std::vector<Point2f>, PafColor>>
ShapeMap::getAllPolygonsWithColour(const std::set<int> &selSet) {
    std::vector<std::pair<std::vector<Point2f>, PafColor>> colouredPolygons;
    std::map<int, SalaShape> &allShapes = getAllShapes();
    for (auto &refShape : allShapes) {
        SalaShape &shape = refShape.second;
        if (shape.isPolygon()) {
            std::vector<Point2f> vertices;
            for (size_t n = 0; n < shape.points.size(); n++) {
                vertices.push_back(shape.points[n]);
            }
            vertices.push_back(shape.points.back());
            PafColor colour(dXreimpl::getDisplayColor(
                AttributeKey(refShape.first), m_attributes->getRow(AttributeKey(refShape.first)),
                *m_attribHandle.get(), selSet, true));
            colouredPolygons.push_back(std::make_pair(vertices, colour));
        }
    }
    return colouredPolygons;
}

std::vector<std::pair<Point2f, PafColor>>
ShapeMap::getAllPointsWithColour(const std::set<int> &selSet) {
    std::vector<std::pair<Point2f, PafColor>> colouredPoints;
    std::map<int, SalaShape> &allShapes = getAllShapes();
    for (auto &refShape : allShapes) {
        SalaShape &shape = refShape.second;
        if (shape.isPoint()) {
            PafColor colour(dXreimpl::getDisplayColor(
                AttributeKey(refShape.first), m_attributes->getRow(AttributeKey(refShape.first)),
                *m_attribHandle.get(), selSet, true));
            colouredPoints.push_back(std::make_pair(shape.getCentroid(), colour));
        }
    }
    return colouredPoints;
}

std::vector<size_t> ShapeMap::makeViewportShapes(const Region4f &viewport) const {

    PixelRef bl = pixelate(viewport.bottomLeft);
    PixelRef tr = pixelate(viewport.topRight);

    std::vector<size_t> displayShapes(m_shapes.size(), static_cast<size_t>(-1));
    for (int i = bl.x; i <= tr.x; i++) {
        for (int j = bl.y; j <= tr.y; j++) {
            const std::vector<ShapeRef> &shapeRefs =
                m_pixelShapes(static_cast<size_t>(j), static_cast<size_t>(i));
            for (const ShapeRef &shape : shapeRefs) {
                // copy the index to the correct draworder position (draworder is
                // formatted on display attribute)
                auto x = std::distance(m_shapes.begin(),
                                       m_shapes.find(static_cast<int>(shape.shapeRef)));
                AttributeKey shapeRefKey(static_cast<int>(shape.shapeRef));
                if (isObjectVisible(m_layers, m_attributes->getRow(shapeRefKey))) {
                    auto shapeIdx = m_attribHandle->findInIndex(shapeRefKey);
                    if (shapeIdx == -1) {
                        throw new genlib::RuntimeException(
                            "Shape " + std::to_string(shape.shapeRef) +
                            " not found when making viewport shapes");
                    }
                    displayShapes[static_cast<size_t>(shapeIdx)] = static_cast<size_t>(x);
                }
            }
        }
    }
    return displayShapes;
}
