// SPDX-FileCopyrightText: 2017 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importutils.hpp"

#include "genlib/stringutils.hpp"
#include "parsers/ntfp.hpp"
#include "parsers/tigerp.hpp"

#include <sstream>

namespace depthmapX {

    const int DXFCIRCLERES = 36;

    class ImportError : public BaseException {
      public:
        ImportError(std::string message) : BaseException(std::move(message)) {}
    };

    std::vector<ShapeMap> importFile(std::istream &stream, Communicator *communicator,
                                     std::string name, ImportType mapType,
                                     ImportFileType fileType) {

        bool parsed = false;
        std::vector<ShapeMap> maps;

        switch (fileType) {
        case CSV: {
            auto &shapeMap = maps.emplace_back(name, mapType == depthmapX::ImportType::DATAMAP
                                                         ? ShapeMap::DATAMAP
                                                         : ShapeMap::DRAWINGMAP);
            parsed = importTxt(shapeMap, stream, ',');

            break;
        }
        case TSV: {
            auto &shapeMap = maps.emplace_back(name, mapType == depthmapX::ImportType::DATAMAP
                                                         ? ShapeMap::DATAMAP
                                                         : ShapeMap::DRAWINGMAP);

            parsed = importTxt(shapeMap, stream, '\t');

            break;
        }
        case CAT: {
            // separate the stream and the communicator, allowing non-file streams read
            return loadCat(communicator->getInFileStream(), communicator, mapType);
        }
        case RT1: {
            // separate the stream and the communicator, allowing non-file streams read
            return loadRT1(communicator->GetFileSet(), communicator, mapType);
        }
        case NTF: {

            NtfMap map;

            try {
                map.open(communicator->GetFileSet(), communicator);
            } catch (std::invalid_argument &) {
                throw new ImportError("Invalid argument when parsing file");
            } catch (std::out_of_range &) {
                throw new ImportError("Out of range error when parsing file");
            }

            if (communicator->IsCancelled()) {
                return maps;
            }

            for (auto layer : map.layers) {

                auto &shapeMap =
                    maps.emplace_back(layer.getName(), mapType == depthmapX::ImportType::DATAMAP
                                                           ? ShapeMap::DATAMAP
                                                           : ShapeMap::DRAWINGMAP);
                shapeMap.init(layer.getLineCount(), map.getRegion());

                for (const auto &geometry : layer.geometries) {
                    for (const auto &line : geometry.lines) {
                        shapeMap.makeLineShape(line);
                    }
                }
            }
            break;
        }
        case DXF: {

            DxfParser dp;

            if (communicator) {
                dp = DxfParser(communicator);

                try {
                    stream >> dp;
                } catch (std::logic_error &) {
                    throw new ImportError("Logic error when parsing file");
                }

                if (communicator->IsCancelled()) {
                    return maps;
                }
            } else {
                dp.open(stream);
            }

            for (auto &layer : dp.getLayers()) {

                auto &shapeMap = maps.emplace_back(
                    layer.first, mapType == depthmapX::ImportType::DATAMAP ? ShapeMap::DATAMAP
                                                                           : ShapeMap::DRAWINGMAP);

                const DxfLayer &dxfLayer = layer.second;

                if (dxfLayer.empty()) {
                    continue;
                }

                parsed = importDxfLayer(dxfLayer, shapeMap);
            }
            break;
        }
        }

        if (parsed) {
            return maps;
        } else {
            return std::vector<ShapeMap>();
        }
    }

    std::vector<ShapeMap> loadCat(std::istream &stream, Communicator *communicator,
                                  ImportType mapType) {
        if (communicator) {
            long size = communicator->GetInfileSize();
            communicator->CommPostMessage(Communicator::NUM_RECORDS, static_cast<size_t>(size));
        }

        time_t atime = 0;

        qtimer(atime, 0);

        size_t size = 0;
        size_t numlines = 0;
        int parsing = 0;
        bool first = true;

        Point2f currentPoint, minPoint, maxPoint;

        while (!stream.eof()) {

            std::string inputline;
            stream >> inputline;
            if (inputline.length() > 1 && inputline[0] != '#') {
                if (!parsing) {
                    if (dXstring::toLower(inputline) == "begin polygon") {
                        parsing = 1;
                    } else if (dXstring::toLower(inputline) == "begin polyline") {
                        parsing = 2;
                    }
                } else if (dXstring::toLower(inputline).substr(0, 3) == "end") {
                    parsing = 0;
                } else {
                    auto tokens = dXstring::split(inputline, ' ', true);
                    currentPoint.x = stod(tokens[0]);
                    currentPoint.y = stod(tokens[1]);
                    numlines++;
                    if (first) {
                        minPoint = currentPoint;
                        maxPoint = currentPoint;
                        first = false;
                    } else {
                        if (currentPoint.x < minPoint.x) {
                            minPoint.x = currentPoint.x;
                        }
                        if (currentPoint.y < minPoint.y) {
                            minPoint.y = currentPoint.y;
                        }
                        if (currentPoint.x > maxPoint.x) {
                            maxPoint.x = currentPoint.x;
                        }
                        if (currentPoint.y > maxPoint.y) {
                            maxPoint.y = currentPoint.y;
                        }
                    }
                }
            }
        }

        std::vector<ShapeMap> maps;

        auto &shapeMap = maps.emplace_back("CATDATA", mapType == depthmapX::ImportType::DATAMAP
                                                          ? ShapeMap::DATAMAP
                                                          : ShapeMap::DRAWINGMAP);

        shapeMap.init(numlines, Region4f(minPoint, maxPoint));

        // in MSVC 6, ios::eof remains set and it needs to be cleared.
        // in MSVC 8 it's even worse: it won't even seekg until eof flag has been
        // cleared
        stream.clear();
        stream.seekg(0, std::ios::beg);

        parsing = 0;
        std::vector<Point2f> points;

        while (!stream.eof()) {

            std::string inputline;
            stream >> inputline;

            if (inputline.length() > 1 && inputline[0] != '#') {
                if (!parsing) {
                    if (dXstring::toLower(inputline) == "begin polygon") {
                        parsing = 1;
                    } else if (dXstring::toLower(inputline) == "begin polyline") {
                        parsing = 2;
                    }
                } else if (dXstring::toLower(inputline).substr(0, 3) == "end") {
                    if (points.size() > 2) {
                        if (parsing == 1) { // polygon
                            shapeMap.makePolyShape(points, false);
                        } else { // polyline
                            shapeMap.makePolyShape(points, true);
                        }
                    } else if (points.size() == 2) {
                        shapeMap.makeLineShape(Line4f(points[0], points[1]));
                    }
                    points.clear();
                    parsing = 0;
                } else {
                    auto tokens = dXstring::split(inputline, ' ', true);
                    currentPoint.x = stod(tokens[0]);
                    currentPoint.y = stod(tokens[1]);
                    points.push_back(currentPoint);
                }
            }

            size += inputline.length() + 1;

            if (communicator) {
                if (qtimer(atime, 500)) {
                    if (communicator->IsCancelled()) {
                        throw Communicator::CancelledException();
                    }
                    communicator->CommPostMessage(Communicator::CURRENT_RECORD, size);
                }
            }
        }

        return maps;
    }

    std::vector<ShapeMap> loadRT1(const std::vector<std::string> &fileset,
                                  Communicator *communicator, ImportType mapType) {
        TigerMap map;

        try {
            map.parse(fileset, communicator);
        } catch (std::invalid_argument &) {
            throw new ImportError("Invalid argument when parsing file");
        } catch (std::out_of_range &) {
            throw new ImportError("Out of range error when parsing file");
        }

        std::vector<ShapeMap> maps;

        if (communicator->IsCancelled()) {
            return maps;
        }

        // for each category
        for (const auto &val : map.categories) {

            auto &shapeMap = maps.emplace_back(val.first, mapType == depthmapX::ImportType::DATAMAP
                                                              ? ShapeMap::DATAMAP
                                                              : ShapeMap::DRAWINGMAP);
            shapeMap.init(val.second.chains.size(), map.getRegion());

            // for each chains in category:
            for (size_t j = 0; j < val.second.chains.size(); j++) {
                // for each node pair in each category
                for (size_t k = 0; k < val.second.chains[j].lines.size(); k++) {
                    shapeMap.makeLineShape(val.second.chains[j].lines[k]);
                }
            }
        }

        return maps;
    }

    bool importTxt(ShapeMap &shapeMap, std::istream &stream, char delimiter = '\t') {
        Table table = csvToTable(stream, delimiter);
        std::vector<std::string> columns;
        int xcol = -1, ycol = -1, x1col = -1, y1col = -1, x2col = -1, y2col = -1, refcol = -1;
        for (auto const &column : table) {
            if (column.first == "x" || column.first == "easting")
                xcol = static_cast<int>(columns.size());
            else if (column.first == "y" || column.first == "northing")
                ycol = static_cast<int>(columns.size());
            else if (column.first == "x1")
                x1col = static_cast<int>(columns.size());
            else if (column.first == "x2")
                x2col = static_cast<int>(columns.size());
            else if (column.first == "y1")
                y1col = static_cast<int>(columns.size());
            else if (column.first == "y2")
                y2col = static_cast<int>(columns.size());
            else if (column.first == "Ref")
                refcol = static_cast<int>(columns.size());
            columns.push_back(column.first);
        }

        if (xcol != -1 && ycol != -1 && refcol != -1) {
            std::map<int, Point2f> points =
                extractPointsWithRefs(table[columns[static_cast<size_t>(xcol)]],
                                      table[columns[static_cast<size_t>(ycol)]],
                                      table[columns[static_cast<size_t>(refcol)]]);
            table.erase(table.find(columns[static_cast<size_t>(xcol)]));
            table.erase(table.find(columns[static_cast<size_t>(ycol)]));
            table.erase(table.find(columns[static_cast<size_t>(refcol)]));

            Region4f region;

            for (auto &point : points) {
                if (region.atZero()) {
                    region = point.second;
                } else {
                    region = region.runion(point.second);
                }
            }

            shapeMap.init(points.size(), region);
            shapeMap.importPointsWithRefs(points, table);

        } else if (xcol != -1 && ycol != -1) {
            std::vector<Point2f> points = extractPoints(table[columns[static_cast<size_t>(xcol)]],
                                                        table[columns[static_cast<size_t>(ycol)]]);
            table.erase(table.find(columns[static_cast<size_t>(xcol)]));
            table.erase(table.find(columns[static_cast<size_t>(ycol)]));

            Region4f region;

            for (auto &point : points) {
                if (region.atZero()) {
                    region = point;
                } else {
                    region = region.runion(point);
                }
            }

            shapeMap.init(points.size(), region);
            shapeMap.importPoints(points, table);

        } else if (x1col != -1 && y1col != -1 && x2col != -1 && y2col != -1 && refcol != -1) {
            std::map<int, Line4f> lines =
                extractLinesWithRef(table[columns[static_cast<size_t>(x1col)]],
                                    table[columns[static_cast<size_t>(y1col)]],
                                    table[columns[static_cast<size_t>(x2col)]],
                                    table[columns[static_cast<size_t>(y2col)]],
                                    table[columns[static_cast<size_t>(refcol)]]);
            table.erase(table.find(columns[static_cast<size_t>(x1col)]));
            table.erase(table.find(columns[static_cast<size_t>(y1col)]));
            table.erase(table.find(columns[static_cast<size_t>(x2col)]));
            table.erase(table.find(columns[static_cast<size_t>(y2col)]));
            table.erase(table.find(columns[static_cast<size_t>(refcol)]));

            Region4f region;

            for (auto &line : lines) {
                if (region.atZero()) {
                    region = line.second;
                } else {
                    region = region.runion(line.second);
                }
            }

            shapeMap.init(lines.size(), region);
            shapeMap.importLinesWithRefs(lines, table);

        } else if (x1col != -1 && y1col != -1 && x2col != -1 && y2col != -1) {
            std::vector<Line4f> lines = extractLines(table[columns[static_cast<size_t>(x1col)]],
                                                     table[columns[static_cast<size_t>(y1col)]],
                                                     table[columns[static_cast<size_t>(x2col)]],
                                                     table[columns[static_cast<size_t>(y2col)]]);
            table.erase(table.find(columns[static_cast<size_t>(x1col)]));
            table.erase(table.find(columns[static_cast<size_t>(y1col)]));
            table.erase(table.find(columns[static_cast<size_t>(x2col)]));
            table.erase(table.find(columns[static_cast<size_t>(y2col)]));

            Region4f region;

            for (auto &line : lines) {
                if (region.atZero()) {
                    region = line;
                } else {
                    region = region.runion(line);
                }
            }

            shapeMap.init(lines.size(), region);
            shapeMap.importLines(lines, table);
        }
        return true;
    }

    Table csvToTable(std::istream &stream, char delimiter = '\t') {

        Table table;
        std::vector<std::string> columns;

        std::string inputline;
        std::getline(stream, inputline);

        // check for a matching delimited header line...
        auto strings = dXstring::split(inputline, delimiter);
        if (strings.size() < 2) {
            // throw exception
            return table;
        }

        for (auto &columnName : strings) {
            if (!columnName.empty()) {
                dXstring::ltrim(columnName, '\"');
                dXstring::rtrim(columnName, '\"');
            }
            table.insert(std::make_pair(columnName, std::vector<std::string>()));
            columns.push_back(columnName);
        }

        while (!stream.eof()) {
            std::getline(stream, inputline);
            if (!inputline.empty()) {
                auto inputStrings = dXstring::split(inputline, delimiter);
                if (inputStrings.size() != columns.size()) {
                    std::stringstream message;
                    message << "Cells in line " << inputline
                            << " not the same number as the columns" << std::flush;
                    throw RuntimeException(message.str().c_str());
                }
                if (!inputStrings.size()) {
                    continue;
                }
                for (size_t i = 0; i < inputStrings.size(); i++) {
                    table[columns[i]].push_back(inputStrings[i]);
                }
            }
        }
        return table;
    }

    std::vector<Line4f> extractLines(ColumnData &x1col, ColumnData &y1col, ColumnData &x2col,
                                     ColumnData &y2col) {
        std::vector<Line4f> lines;
        for (size_t i = 0; i < x1col.size(); i++) {
            double x1 = stod(x1col[i]);
            double y1 = stod(y1col[i]);
            double x2 = stod(x2col[i]);
            double y2 = stod(y2col[i]);
            lines.push_back(Line4f(Point2f(x1, y1), Point2f(x2, y2)));
        }
        return lines;
    }

    std::map<int, Line4f> extractLinesWithRef(ColumnData &x1col, ColumnData &y1col,
                                              ColumnData &x2col, ColumnData &y2col,
                                              ColumnData &refcol) {
        std::map<int, Line4f> lines;
        for (size_t i = 0; i < x1col.size(); i++) {
            double x1 = stod(x1col[i]);
            double y1 = stod(y1col[i]);
            double x2 = stod(x2col[i]);
            double y2 = stod(y2col[i]);
            int ref = stoi(refcol[i]);
            lines.insert(std::make_pair(ref, Line4f(Point2f(x1, y1), Point2f(x2, y2))));
        }
        return lines;
    }
    std::vector<Point2f> extractPoints(ColumnData &x, ColumnData &y) {
        std::vector<Point2f> points;
        for (size_t i = 0; i < x.size(); i++) {
            points.push_back(Point2f(stod(x[i]), stod(y[i])));
        }
        return points;
    }
    std::map<int, Point2f> extractPointsWithRefs(ColumnData &x, ColumnData &y, ColumnData &ref) {
        std::map<int, Point2f> points;
        for (size_t i = 0; i < x.size(); i++) {
            points.insert(std::make_pair(stoi(ref[i]), Point2f(stod(x[i]), stod(y[i]))));
        }
        return points;
    }

    bool importDxfLayer(const DxfLayer &dxfLayer, ShapeMap &shapeMap) {
        std::vector<Point2f> points;
        std::vector<Line4f> lines;
        std::vector<Polyline> polylines;

        for (size_t jp = 0; jp < dxfLayer.numPoints(); jp++) {
            const DxfVertex &dxfPoint = dxfLayer.getPoint(jp);
            points.push_back(Point2f(dxfPoint.x, dxfPoint.y));
        }

        for (size_t j = 0; j < dxfLayer.numLines(); j++) {
            const DxfLine &dxfLine = dxfLayer.getLine(j);
            Line4f line = Line4f(Point2f(dxfLine.getStart().x, dxfLine.getStart().y),
                                 Point2f(dxfLine.getEnd().x, dxfLine.getEnd().y));
            lines.push_back(line);
        }

        for (size_t k = 0; k < dxfLayer.numPolyLines(); k++) {
            const DxfPolyLine &poly = dxfLayer.getPolyLine(k);
            std::vector<Point2f> vertices;
            for (size_t m = 0; m < poly.numVertices(); m++) {
                DxfVertex v = poly.getVertex(m);
                vertices.push_back(Point2f(v.x, v.y));
            }
            polylines.push_back(depthmapX::Polyline(std::move(vertices),
                                                    (poly.getAttributes() & DxfPolyLine::CLOSED) ==
                                                        DxfPolyLine::CLOSED));
        }

        for (size_t l = 0; l < dxfLayer.numSplines(); l++) {
            const DxfSpline &poly = dxfLayer.getSpline(l);
            std::vector<Point2f> vertices;
            for (size_t m = 0; m < poly.numVertices(); m++) {
                DxfVertex v = poly.getVertex(m);
                vertices.push_back(Point2f(v.x, v.y));
            }
            polylines.push_back(depthmapX::Polyline(std::move(vertices),
                                                    (poly.getAttributes() & DxfPolyLine::CLOSED) ==
                                                        DxfPolyLine::CLOSED));
        }

        for (size_t n = 0; n < dxfLayer.numArcs(); n++) {
            const DxfArc &circ = dxfLayer.getArc(n);
            std::vector<Point2f> vertices;
            int segments = static_cast<int>(circ.numSegments(DXFCIRCLERES));
            if (segments > 1) {
                for (int m = 0; m <= segments; m++) {
                    DxfVertex v = circ.getVertex(m, segments);
                    vertices.push_back(Point2f(v.x, v.y));
                }
            }
            polylines.push_back(depthmapX::Polyline(std::move(vertices), false));
        }

        for (size_t n = 0; n < dxfLayer.numEllipses(); n++) {
            const DxfEllipse &ellipse = dxfLayer.getEllipse(n);
            std::vector<Point2f> vertices;
            auto segments = static_cast<int>(ellipse.numSegments(DXFCIRCLERES));
            if (segments > 1) {
                for (int m = 0; m <= segments; m++) {
                    DxfVertex v = ellipse.getVertex(m, segments);
                    vertices.push_back(Point2f(v.x, v.y));
                }
            }
            polylines.push_back(depthmapX::Polyline(std::move(vertices), false));
        }

        for (size_t nc = 0; nc < dxfLayer.numCircles(); nc++) {
            const DxfCircle &circ = dxfLayer.getCircle(nc);
            std::vector<Point2f> vertices;
            for (int m = 0; m < DXFCIRCLERES; m++) {
                DxfVertex v = circ.getVertex(m, DXFCIRCLERES);
                vertices.push_back(Point2f(v.x, v.y));
            }
            polylines.push_back(depthmapX::Polyline(std::move(vertices), true));
        }
        DxfVertex layerMin = dxfLayer.getExtMin();
        DxfVertex layerMax = dxfLayer.getExtMax();

        Region4f region =
            Region4f(Point2f(layerMin.x, layerMin.y), Point2f(layerMax.x, layerMax.y));

        shapeMap.init(points.size() + lines.size() + polylines.size(), region);
        // parameters could be passed in the Table here such as the
        // layer/block/colour/linetype etc.
        shapeMap.importPoints(points, Table());
        shapeMap.importLines(lines, Table());
        shapeMap.importPolylines(polylines, Table());

        return true;
    }

    bool importAttributes(AttributeTable &attributes, std::istream &stream, char delimiter = '\t') {
        Table table = csvToTable(stream, delimiter);
        std::vector<std::string> outColumns;
        int refcol = -1;
        for (auto const &column : table) {
            if (column.first == "Ref")
                refcol = static_cast<int>(outColumns.size());
            else
                outColumns.push_back(column.first);
        }
        if (table.size() == 0) {
            throw RuntimeException("No usable data found in file");
        }
        if (refcol == -1) {
            throw RuntimeException("The \"Ref\" column is reqired");
        }
        if (outColumns.size() < 1) {
            throw RuntimeException("No data found to join");
        }
        std::vector<AttributeRow *> inRows;
        for (const auto &refInFile : table["Ref"]) {
            int ref = std::stoi(refInFile);
            auto iter = attributes.find(AttributeKey(ref));
            if (iter == attributes.end()) {
                std::stringstream message;
                message << "Key " << ref << "not found in attribute table" << std::flush;
                throw RuntimeException(message.str().c_str());
            }
            inRows.push_back(&(iter->getRow()));
        }

        // Up until this point we have not touched the attribute table
        // so no need for corrective measures yet

        for (const std::string &column : outColumns) {
            auto colIdx = attributes.insertOrResetColumn(column);
            auto outRowIter = table[column].begin();
            for (auto inRowIter = inRows.begin(); inRowIter != inRows.end();
                 inRowIter++, outRowIter++) {
                (*inRowIter)->setValue(colIdx, std::stof(*outRowIter));
            }
        }
        return true;
    }
} // namespace depthmapX
