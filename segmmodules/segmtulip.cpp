// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtulip.h"

#include "genlib/stringutils.h"

std::vector<std::string> SegmentTulip::getRequiredColumns(ShapeGraph &map,
                                                          std::vector<double> radii) {
    std::vector<std::string> newColumns;
    std::optional<std::string> weighting_col_text2 = std::nullopt;
    if (m_weighted_measure_col2 != -1) {
        weighting_col_text2 = map.getAttributeTable().getColumnName(m_weighted_measure_col2);
    }

    std::optional<std::string> routeweight_col_text = std::nullopt;
    if (m_routeweight_col != -1) {
        routeweight_col_text = map.getAttributeTable().getColumnName(m_routeweight_col);
    }

    std::optional<std::string> weighting_col_text = std::nullopt;
    if (m_weighted_measure_col != -1) {
        weighting_col_text = map.getAttributeTable().getColumnName(m_weighted_measure_col);
    }
    for (auto radius : radii) {
        if (!m_forceLegacyColumnOrder) {
            if (m_choice) {
                // EF routeweight *
                if (m_routeweight_col != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, m_tulip_bins, m_radius_type, radius, routeweight_col_text));
                    if (m_weighted_measure_col != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                            routeweight_col_text, weighting_col_text));
                    }
                    // EFEF*
                    if (m_weighted_measure_col2 != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                            routeweight_col_text, weighting_col_text, weighting_col_text2));
                    }
                    //*EFEF
                }
                //*EF routeweight
                else {                                       // Normal run // TV
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, m_tulip_bins, m_radius_type, radius));
                    if (m_weighted_measure_col != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius, std::nullopt,
                            weighting_col_text));
                    }
                    // EFEF*
                    if (m_weighted_measure_col2 != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius, std::nullopt,
                            weighting_col_text, weighting_col_text2));
                    }
                    //*EFEF
                }
            }

            // EF routeweight *
            if (m_routeweight_col != -1) {
                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text)); // <- note, the fact this is a tulip is unnecessary

                newColumns.push_back(getFormattedColumn( //
                    Column::NODE_COUNT, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text)); // <- note, the fact this is a tulip is unnecessary
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text)); // <- note, the fact this is a tulip is unnecessary

                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL, m_tulip_bins, m_radius_type, radius, routeweight_col_text));

                if (m_weighted_measure_col != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::INTEGRATION, m_tulip_bins, m_radius_type, radius,
                        routeweight_col_text, weighting_col_text));
                    // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                    newColumns.push_back(getFormattedColumn( //
                        Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius,
                        routeweight_col_text, weighting_col_text));
                }

            }
            //*EF routeweight
            else { // Normal run // TV

                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, m_tulip_bins, m_radius_type,
                    radius)); // <- note, the fact this is a tulip is unnecessary

                newColumns.push_back(getFormattedColumn( //
                    Column::NODE_COUNT, m_tulip_bins, m_radius_type,
                    radius)); // <- note, the fact this is a tulip is unnecessary
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type,
                    radius)); // <- note, the fact this is a tulip is unnecessary

                if (m_weighted_measure_col != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::INTEGRATION, m_tulip_bins, m_radius_type, radius, std::nullopt,
                        weighting_col_text)); // <- note, the fact this is a tulip is unnecessary
                    // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                    newColumns.push_back(getFormattedColumn( //
                        Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius, std::nullopt,
                        weighting_col_text));
                    newColumns.push_back(getFormattedColumn( //
                        Column::TOTAL, m_tulip_bins, m_radius_type, radius, std::nullopt,
                        weighting_col_text));
                }
            }
        } else {

            if (m_choice) {
                // EF routeweight *
                if (m_routeweight_col != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, m_tulip_bins, m_radius_type, radius, routeweight_col_text));
                    if (m_weighted_measure_col != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                            routeweight_col_text, weighting_col_text));
                    }
                    // EFEF*
                    if (m_weighted_measure_col2 != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                            routeweight_col_text, weighting_col_text, weighting_col_text2));
                    }
                    //*EFEF
                }
                //*EF routeweight
                else {                                       // Normal run // TV
                    newColumns.push_back(getFormattedColumn( //
                        Column::CHOICE, m_tulip_bins, m_radius_type, radius));
                    if (m_weighted_measure_col != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius, std::nullopt,
                            weighting_col_text));
                    }
                    // EFEF*
                    if (m_weighted_measure_col2 != -1) {
                        newColumns.push_back(getFormattedColumn( //
                            Column::CHOICE, m_tulip_bins, m_radius_type, radius, std::nullopt,
                            weighting_col_text, weighting_col_text2));
                    }
                    //*EFEF
                }
            }

            // EF routeweight *
            if (m_routeweight_col != -1) {
                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text)); // <- note, the fact this is a tulip is unnecessary
                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, m_tulip_bins, m_radius_type, radius, routeweight_col_text,
                    weighting_col_text));

                newColumns.push_back(getFormattedColumn( //
                    Column::NODE_COUNT, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text)); // <- note, the fact this is a tulip is unnecessary
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text)); // <- note, the fact this is a tulip is unnecessary

                // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius, routeweight_col_text,
                    weighting_col_text));
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL, m_tulip_bins, m_radius_type, radius, routeweight_col_text));

            }
            //*EF routeweight
            else { // Normal run // TV

                newColumns.push_back(getFormattedColumn( //
                    Column::INTEGRATION, m_tulip_bins, m_radius_type,
                    radius)); // <- note, the fact this is a tulip is unnecessary

                newColumns.push_back(getFormattedColumn( //
                    Column::NODE_COUNT, m_tulip_bins, m_radius_type,
                    radius)); // <- note, the fact this is a tulip is unnecessary
                newColumns.push_back(getFormattedColumn( //
                    Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type,
                    radius)); // <- note, the fact this is a tulip is unnecessary

                if (m_weighted_measure_col != -1) {
                    newColumns.push_back(getFormattedColumn( //
                        Column::INTEGRATION, m_tulip_bins, m_radius_type, radius, std::nullopt,
                        weighting_col_text)); // <- note, the fact this is a tulip is unnecessary
                    // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                    newColumns.push_back(getFormattedColumn( //
                        Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius, std::nullopt,
                        weighting_col_text));
                    newColumns.push_back(getFormattedColumn( //
                        Column::TOTAL, m_tulip_bins, m_radius_type, radius, std::nullopt,
                        weighting_col_text));
                }
            }
        }
    }
    return newColumns;
}

AnalysisResult SegmentTulip::run(Communicator *comm, ShapeGraph &map, bool) {

    AnalysisResult result;
    if (map.getMapType() != ShapeMap::SEGMENTMAP) {
        return result;
    }

    // TODO: Understand what these parameters do. They were never truly provided in the original
    // function
    int weighting_col2 = m_weighted_measure_col2;
    int routeweight_col = m_routeweight_col;
    bool interactive = m_interactive;

    AttributeTable &attributes = map.getAttributeTable();

    int processed_rows = 0;

    time_t atime = 0;

    if (comm) {
        qtimer(atime, 0);
        comm->CommPostMessage(
            Communicator::NUM_RECORDS,
            (m_selSet.has_value() ? m_selSet->size() : map.getConnections().size()));
    }

    // note: radius must be sorted lowest to highest, but if -1 occurs ("radius n") it needs to be
    // last...
    // ...to ensure no mess ups, we'll re-sort here:
    bool radius_n = false;
    std::vector<double> radius_unconverted;
    for (int radius : m_radius_set) {
        if (radius == -1.0) {
            radius_n = true;
        } else {
            radius_unconverted.push_back(radius);
        }
    }
    if (radius_n) {
        radius_unconverted.push_back(-1.0);
    }

    // retrieve weighted col data, as this may well be overwritten in the new analysis:
    std::vector<float> weights;
    std::vector<float> routeweights; // EF
    std::string weighting_col_text;

    int tulip_bins = m_tulip_bins;

    if (m_weighted_measure_col != -1) {
        weighting_col_text = attributes.getColumnName(m_weighted_measure_col);
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights.push_back(
                map.getAttributeRowFromShapeIndex(i).getValue(m_weighted_measure_col));
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights.push_back(1.0f);
        }
    }
    // EF routeweight*
    std::string routeweight_col_text;
    if (routeweight_col != -1) {
        // we normalise the column values between 0 and 1 and reverse it so that high values can be
        // treated as a 'low cost' - similar to the angular cost
        double max_value = attributes.getColumn(routeweight_col).getStats().max;
        routeweight_col_text = attributes.getColumnName(routeweight_col);
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            routeweights.push_back(1.0 -
                                   (map.getAttributeRowFromShapeIndex(i).getValue(routeweight_col) /
                                    max_value)); // scale and revert!
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            routeweights.push_back(1.0f);
        }
    }
    //*EF routeweight

    // EFEF*
    // for origin-destination weighting
    std::vector<float> weights2;
    std::string weighting_col_text2;
    if (weighting_col2 != -1) {
        weighting_col_text2 = attributes.getColumnName(weighting_col2);
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights2.push_back(map.getAttributeRowFromShapeIndex(i).getValue(weighting_col2));
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights2.push_back(1.0f);
        }
    }
    //*EFEF

    auto newColumns = getRequiredColumns(map, radius_unconverted);
    for (auto &col : newColumns) {
        attributes.insertOrResetColumn(col);
        result.addAttribute(col);
    }

    std::string tulip_text = std::string("T") + dXstring::formatString(tulip_bins, "%d");

    std::vector<size_t> choice_col, w_choice_col, w_choice_col2, count_col, integ_col, w_integ_col,
        td_col, w_td_col, total_weight_col;
    // then look them up! eek....
    for (auto radius : radius_unconverted) {
        std::string radius_text = makeRadiusText(m_radius_type, radius);
        if (m_choice) {
            // EF routeweight *
            if (routeweight_col != -1) {
                choice_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text));
                if (m_weighted_measure_col != -1) {
                    w_choice_col.push_back(getFormattedColumnIdx( //
                        attributes, Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                        routeweight_col_text, weighting_col_text));
                }
                // EFEF*
                if (weighting_col2 != -1) {
                    w_choice_col2.push_back(getFormattedColumnIdx( //
                        attributes, Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                        routeweight_col_text, weighting_col_text, weighting_col_text2));
                }
                //*EFEF
            }
            //* EF routeweight
            else { // Normal run // TV

                choice_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::CHOICE, m_tulip_bins, m_radius_type, radius));
                if (m_weighted_measure_col != -1) {
                    w_choice_col.push_back(getFormattedColumnIdx( //
                        attributes, Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                        std::nullopt, weighting_col_text));
                }
                // EFEF*
                if (weighting_col2 != -1) {
                    w_choice_col2.push_back(getFormattedColumnIdx( //
                        attributes, Column::CHOICE, m_tulip_bins, m_radius_type, radius,
                        std::nullopt, weighting_col_text, weighting_col_text2));
                }
                //*EFEF
            }
        }
        // EF routeweight *
        if (routeweight_col != -1) {

            integ_col.push_back(getFormattedColumnIdx( //
                attributes, Column::INTEGRATION, m_tulip_bins, m_radius_type, radius,
                routeweight_col_text));
            count_col.push_back(getFormattedColumnIdx( //
                attributes, Column::NODE_COUNT, m_tulip_bins, m_radius_type, radius,
                routeweight_col_text));
            td_col.push_back(getFormattedColumnIdx( //
                attributes, Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius,
                routeweight_col_text));
            if (m_weighted_measure_col != -1) {
                // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                w_integ_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::INTEGRATION, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text, weighting_col_text));
                w_td_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text, weighting_col_text));
                total_weight_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::TOTAL, m_tulip_bins, m_radius_type, radius,
                    routeweight_col_text));
            }
        }
        //* EF routeweight
        else {                                         // Normal run // TV
            integ_col.push_back(getFormattedColumnIdx( //
                attributes, Column::INTEGRATION, m_tulip_bins, m_radius_type, radius));
            count_col.push_back(getFormattedColumnIdx( //
                attributes, Column::NODE_COUNT, m_tulip_bins, m_radius_type, radius));
            td_col.push_back(getFormattedColumnIdx( //
                attributes, Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius));
            if (m_weighted_measure_col != -1) {
                // '[' comes after 'R' in ASCII, so this column will come after Mean Depth R...
                w_integ_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::INTEGRATION, m_tulip_bins, m_radius_type, radius,
                    std::nullopt, weighting_col_text));
                w_td_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::TOTAL_DEPTH, m_tulip_bins, m_radius_type, radius,
                    std::nullopt, weighting_col_text));
                total_weight_col.push_back(getFormattedColumnIdx( //
                    attributes, Column::TOTAL, m_tulip_bins, m_radius_type, radius, std::nullopt,
                    weighting_col_text));
            }
        }
    }

    tulip_bins /= 2; // <- actually use semicircle of tulip bins
    tulip_bins += 1;

    std::vector<std::vector<SegmentData>> bins(tulip_bins);

    // TODO: Replace these with STL
    AnalysisInfo ***audittrail;
    unsigned int **uncovered;
    audittrail = new AnalysisInfo **[map.getConnections().size()];
    uncovered = new unsigned int *[map.getConnections().size()];
    for (size_t i = 0; i < map.getConnections().size(); i++) {
        audittrail[i] = new AnalysisInfo *[radius_unconverted.size()];
        for (size_t j = 0; j < radius_unconverted.size(); j++) {
            audittrail[i][j] = new AnalysisInfo[2];
        }
        uncovered[i] = new unsigned int[2];
    }
    std::vector<double> radius;

    for (auto uradius : radius_unconverted) {
        if (m_radius_type == RadiusType::ANGULAR && uradius != -1) {
            radius.push_back(floor(uradius * tulip_bins * 0.5));
        } else {
            radius.push_back(uradius);
        }
    }
    // entered once for each segment
    int length_col = attributes.getColumnIndex("Segment Length");
    std::vector<float> lengths;
    if (length_col != -1) {
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            AttributeRow &row = map.getAttributeRowFromShapeIndex(i);
            lengths.push_back(row.getValue(length_col));
        }
    }

    auto radiussize = radius.size();
    int radiusmask = 0;
    for (size_t i = 0; i < radiussize; i++) {
        radiusmask |= (1 << i);
    }

    for (size_t cursor = 0; cursor < map.getConnections().size(); cursor++) {
        auto &shapeRef = map.getShapeRefFromIndex(cursor)->first;
        AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(shapeRef));

        if (m_selSet.has_value()) {
            if (m_selSet->find(shapeRef) == m_selSet->end()) {
                continue;
            }
        }

        for (int k = 0; k < tulip_bins; k++) {
            bins[k].clear();
        }
        for (size_t j = 0; j < map.getConnections().size(); j++) {
            for (int dir = 0; dir < 2; dir++) {
                for (size_t k = 0; k < radiussize; k++) {
                    audittrail[j][k][dir].clearLine();
                }
                uncovered[j][dir] = radiusmask;
            }
        }

        double rootseglength = row.getValue(length_col);
        double rootweight = (m_weighted_measure_col != -1) ? weights[cursor] : 0.0;

        // setup: direction 0 (both ways), segment i, previous -1, segdepth (step depth) 0,
        // metricdepth 0.5 * rootseglength, bin 0
        SegmentData segmentData(0, cursor, SegmentRef(), 0, 0.5 * rootseglength, radiusmask);
        auto it = std::lower_bound(bins[0].begin(), bins[0].end(), segmentData);
        if (it == bins[0].end() || segmentData != *it) {
            bins[0].insert(it, segmentData);
        }
        // this version below is only designed to be used temporarily --
        // could be on an option?
        // bins[0].push_back(SegmentData(0,rowid,SegmentRef(),0,0.0,radiusmask));
        int depthlevel = 0;
        int opencount = 1;
        size_t currentbin = 0;
        while (opencount) {
            while (!bins[currentbin].size()) {
                depthlevel++;
                currentbin++;
                if (currentbin == static_cast<size_t>(tulip_bins)) {
                    currentbin = 0;
                }
            }
            SegmentData lineindex = bins[currentbin].back();
            bins[currentbin].pop_back();
            //
            opencount--;

            int ref = lineindex.ref;
            int dir = (lineindex.dir == 1) ? 0 : 1;
            int coverage = lineindex.coverage & uncovered[ref][dir];
            if (coverage != 0) {
                int rbin = 0;
                int rbinbase;
                if (lineindex.previous.ref != -1) {
                    uncovered[ref][dir] &= ~coverage;
                    while (((coverage >> rbin) & 0x1) == 0)
                        rbin++;
                    rbinbase = rbin;
                    while (rbin < static_cast<int>(radiussize)) {
                        if (((coverage >> rbin) & 0x1) == 1) {
                            audittrail[ref][rbin][dir].depth = depthlevel;
                            audittrail[ref][rbin][dir].previous = lineindex.previous;
                            audittrail[lineindex.previous.ref][rbin]
                                      [(lineindex.previous.dir == 1) ? 0 : 1]
                                          .leaf = false;
                        }
                        rbin++;
                    }
                } else {
                    rbinbase = 0;
                    uncovered[ref][0] &= ~coverage;
                    uncovered[ref][1] &= ~coverage;
                }
                Connector &line = map.getConnections()[ref];
                float seglength;
                int extradepth;
                if (lineindex.dir != -1) {
                    for (auto &segconn : line.m_forward_segconns) {
                        rbin = rbinbase;
                        SegmentRef conn = segconn.first;
                        if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
                            // EF routeweight*
                            if (routeweight_col !=
                                -1) { // EF here we do the weighting of the angular cost by the
                                      // weight of the next segment
                                // note that the content of the routeweights array is scaled between
                                // 0 and 1 and is reversed such that: = 1.0-(attributes.getValue(i,
                                // routeweight_col)/max_value)
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5 *
                                                        routeweights[conn.ref]);
                            }
                            //*EF routeweight
                            else {
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                            }
                            seglength = lengths[conn.ref];
                            switch (m_radius_type) {
                            case RadiusType::ANGULAR:
                                while (rbin != static_cast<int>(radiussize) && radius[rbin] != -1 &&
                                       depthlevel + extradepth > (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::METRIC:
                                while (rbin != static_cast<int>(radiussize) && radius[rbin] != -1 &&
                                       lineindex.metricdepth + seglength * 0.5 > radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::TOPOLOGICAL:
                                if (rbin != static_cast<int>(radiussize) && radius[rbin] != -1 &&
                                    lineindex.segdepth >= (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::NONE:
                                break;
                            }
                            if ((coverage >> rbin) != 0) {
                                SegmentData sd(
                                    conn, SegmentRef(1, lineindex.ref), lineindex.segdepth + 1,
                                    lineindex.metricdepth + seglength, (coverage >> rbin) << rbin);
                                size_t bin = (currentbin + tulip_bins + extradepth) % tulip_bins;
                                depthmapX::insert_sorted(bins[bin], sd);
                                opencount++;
                            }
                        }
                    }
                }
                if (lineindex.dir != 1) {
                    for (auto &segconn : line.m_back_segconns) {
                        rbin = rbinbase;
                        SegmentRef conn = segconn.first;
                        if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
                            // EF routeweight*
                            if (routeweight_col !=
                                -1) { // EF here we do the weighting of the angular cost by the
                                      // weight of the next segment
                                // note that the content of the routeweights array is scaled between
                                // 0 and 1 and is reversed such that: = 1.0-(attributes.getValue(i,
                                // routeweight_col)/max_value)
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5 *
                                                        routeweights[conn.ref]);
                            }
                            //*EF routeweight
                            else {
                                extradepth = (int)floor(segconn.second * tulip_bins * 0.5);
                            }
                            seglength = lengths[conn.ref];
                            switch (m_radius_type) {
                            case RadiusType::ANGULAR:
                                while (rbin != static_cast<int>(radiussize) && radius[rbin] != -1 &&
                                       depthlevel + extradepth > (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::METRIC:
                                while (rbin != static_cast<int>(radiussize) && radius[rbin] != -1 &&
                                       lineindex.metricdepth + seglength * 0.5 > radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::TOPOLOGICAL:
                                if (rbin != static_cast<int>(radiussize) && radius[rbin] != -1 &&
                                    lineindex.segdepth >= (int)radius[rbin]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::NONE:
                                break;
                            }
                            if ((coverage >> rbin) != 0) {
                                SegmentData sd(
                                    conn, SegmentRef(-1, lineindex.ref), lineindex.segdepth + 1,
                                    lineindex.metricdepth + seglength, (coverage >> rbin) << rbin);
                                size_t bin = (currentbin + tulip_bins + extradepth) % tulip_bins;
                                depthmapX::insert_sorted(bins[bin], sd);
                                opencount++;
                            }
                        }
                    }
                }
            }
        }
        // set the attributes for this node:
        for (size_t k = 0; k < radiussize; k++) {
            // note, curs_total_depth must use double as mantissa can get too long for int in large
            // systems
            double curs_node_count = 0.0, curs_total_depth = 0.0;
            double curs_total_weight = 0.0, curs_total_weighted_depth = 0.0;
            size_t j;
            for (j = 0; j < map.getConnections().size(); j++) {
                // find dir according
                bool m0 = ((uncovered[j][0] >> k) & 0x1) == 0;
                bool m1 = ((uncovered[j][1] >> k) & 0x1) == 0;
                if ((m0 | m1) != 0) {
                    int dir;
                    if (m0 & m1) {
                        // dir is the one with the lowest depth:
                        if (audittrail[j][k][0].depth < audittrail[j][k][1].depth)
                            dir = 0;
                        else
                            dir = 1;
                    } else {
                        // dir is simply the one that's filled in:
                        dir = m0 ? 0 : 1;
                    }
                    curs_node_count++;
                    curs_total_depth += audittrail[j][k][dir].depth;
                    curs_total_weight += weights[j];
                    curs_total_weighted_depth += audittrail[j][k][dir].depth * weights[j];
                    //
                    if (m_choice && audittrail[j][k][dir].leaf) {
                        // note, graph may be directed (e.g., for one way streets), so both ways
                        // must be included from now on:
                        SegmentRef here = SegmentRef(dir == 0 ? 1 : -1, j);
                        if (here.ref != static_cast<int>(cursor)) {
                            int choicecount = 0;
                            double choiceweight = 0.0;
                            // EFEF*
                            double choiceweight2 = 0.0;
                            //*EFEF
                            while (
                                here.ref !=
                                static_cast<int>(
                                    cursor)) { // not rowid means not the current root for the path
                                int heredir = (here.dir == 1) ? 0 : 1;
                                // each node has the existing choicecount and choiceweight from
                                // previously encountered nodes added to it
                                audittrail[here.ref][k][heredir].choice += choicecount;
                                // nb, weighted values calculated anyway to save time on 'if'
                                audittrail[here.ref][k][heredir].weighted_choice += choiceweight;
                                // EFEF*
                                audittrail[here.ref][k][heredir].weighted_choice2 += choiceweight2;
                                //*EFEF
                                // if the node hasn't been encountered before, the choicecount and
                                // choiceweight is incremented for all remaining nodes to be
                                // encountered on the backwards route from it
                                if (!audittrail[here.ref][k][heredir].choicecovered) {
                                    // this node has not been encountered before: this adds the
                                    // choicecount and weight for this node, and flags it as visited
                                    choicecount++;
                                    choiceweight += weights[here.ref] * rootweight;
                                    // EFEF*
                                    choiceweight2 += weights2[here.ref] * rootweight; // rootweight!
                                    //*EFEF

                                    audittrail[here.ref][k][heredir].choicecovered = true;
                                    // note, for weighted choice, the start and end points have
                                    // choice added to them:
                                    if (m_weighted_measure_col != -1) {
                                        audittrail[here.ref][k][heredir].weighted_choice +=
                                            (weights[here.ref] * rootweight) / 2.0;
                                        // EFEF*
                                        if (weighting_col2 != -1) {
                                            audittrail[here.ref][k][heredir].weighted_choice2 +=
                                                (weights2[here.ref] * rootweight) /
                                                2.0; // rootweight!
                                        }
                                        //*EFEF
                                    }
                                }
                                here = audittrail[here.ref][k][heredir].previous;
                            }
                            // note, for weighted choice, the start and end points have choice added
                            // to them: (this is the summed weight for all starting nodes
                            // encountered in this path)
                            if (m_weighted_measure_col != -1) {
                                audittrail[here.ref][k][(here.dir == 1) ? 0 : 1].weighted_choice +=
                                    choiceweight / 2.0;
                                // EFEF*
                                if (weighting_col2 != -1) {
                                    audittrail[here.ref][k][(here.dir == 1) ? 0 : 1]
                                        .weighted_choice2 += choiceweight2 / 2.0;
                                }
                                //*EFEF
                            }
                        }
                    }
                }
            }
            double total_depth_conv = curs_total_depth / ((tulip_bins - 1.0f) * 0.5f);
            double total_weighted_depth_conv =
                curs_total_weighted_depth / ((tulip_bins - 1.0f) * 0.5f);
            //
            row.setValue(count_col[k], float(curs_node_count));
            if (curs_node_count > 1) {
                // for dmap 8 and above, mean depth simply isn't calculated as for radius measures
                // it is meaningless
                row.setValue(td_col[k], total_depth_conv);
                if (m_weighted_measure_col != -1) {
                    row.setValue(total_weight_col[k], float(curs_total_weight));
                    row.setValue(w_td_col[k], float(total_weighted_depth_conv));
                }
            } else {
                row.setValue(td_col[k], -1);
                if (m_weighted_measure_col != -1) {
                    row.setValue(total_weight_col[k], -1.0f);
                    row.setValue(w_td_col[k], -1.0f);
                }
            }
            // for dmap 10 an above, integration is included!
            if (total_depth_conv > 1e-9) {
                row.setValue(integ_col[k],
                             (float)(curs_node_count * curs_node_count / total_depth_conv));
                if (m_weighted_measure_col != -1) {
                    row.setValue(w_integ_col[k], (float)(curs_total_weight * curs_total_weight /
                                                         total_weighted_depth_conv));
                }
            } else {
                row.setValue(integ_col[k], -1);
                if (m_weighted_measure_col != -1) {
                    row.setValue(w_integ_col[k], -1.0f);
                }
            }
        }
        //
        processed_rows++;
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    // interactive is usual Depthmap: throw an exception if cancelled
                    if (interactive) {
                        for (size_t i = 0; i < map.getConnections().size(); i++) {
                            for (size_t j = 0; j < size_t(radiussize); j++) {
                                delete[] audittrail[i][j];
                            }
                            delete[] audittrail[i];
                            delete[] uncovered[i];
                        }
                        delete[] audittrail;
                        delete[] uncovered;
                        throw Communicator::CancelledException();
                    } else {
                        // in non-interactive mode, retain what's been processed already
                        break;
                    }
                }
                comm->CommPostMessage(Communicator::CURRENT_RECORD, cursor);
            }
        }
    }
    if (m_choice) {
        for (size_t cursor = 0; cursor < map.getConnections().size(); cursor++) {
            AttributeRow &row = attributes.getRow(
                AttributeKey(depthmapX::getMapAtIndex(map.getAllShapes(), cursor)->first));
            for (size_t r = 0; r < radius.size(); r++) {
                // according to Eva's correction, total choice and total weighted choice
                // should already have been accumulated by radius at this stage
                double total_choice =
                    audittrail[cursor][r][0].choice + audittrail[cursor][r][1].choice;
                double total_weighted_choice = audittrail[cursor][r][0].weighted_choice +
                                               audittrail[cursor][r][1].weighted_choice;
                // EFEF*
                double total_weighted_choice2 = audittrail[cursor][r][0].weighted_choice2 +
                                                audittrail[cursor][r][1].weighted_choice2;
                //*EFEF

                // normalised choice now excluded for two reasons:
                // a) not useful measure, b) in parallel calculations, cannot be calculated at this
                // stage n.b., it is possible through the front end: the new choice takes into
                // account bidirectional routes, so it should be normalised according to (n-1)(n-2)
                // (maximum possible through routes) not (n-1)(n-2)/2 the relativised segment length
                // weighted choice equation was
                // (total_seg_length*total_seg_length-seg_length*seg_length)/2 again, drop the
                // divide by 2 for the new implementation
                //
                //
                row.setValue(choice_col[r], float(total_choice));
                if (m_weighted_measure_col != -1) {
                    row.setValue(w_choice_col[r], float(total_weighted_choice));
                    // EFEF*
                    if (weighting_col2 != -1) {
                        row.setValue(w_choice_col2[r], float(total_weighted_choice2));
                    }
                    //*EFEF
                }
            }
        }
    }
    for (size_t i = 0; i < map.getConnections().size(); i++) {
        for (size_t j = 0; j < radiussize; j++) {
            delete[] audittrail[i][j];
        }
        delete[] audittrail[i];
        delete[] uncovered[i];
    }
    delete[] audittrail;
    delete[] uncovered;

    result.completed = processed_rows > 0;

    return result;
}
