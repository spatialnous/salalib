// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2017-2024 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "segmtulipleafchoice.hpp"

#include "../genlib/stringutils.hpp"

std::vector<std::string> SegmentTulipLeafChoice::getRequiredColumns(ShapeGraph &map,
                                                                    std::vector<double> radii) {
    std::vector<std::string> newColumns;
    std::optional<std::string> weightingColText2 = std::nullopt;
    if (m_weightedMeasureCol2 != -1) {
        weightingColText2 =
            map.getAttributeTable().getColumnName(static_cast<size_t>(m_weightedMeasureCol2));
    }

    std::optional<std::string> routeweightColText = std::nullopt;
    if (m_routeweightCol != -1) {
        routeweightColText =
            map.getAttributeTable().getColumnName(static_cast<size_t>(m_routeweightCol));
    }

    std::optional<std::string> weightingColText = std::nullopt;
    if (m_weightedMeasureCol != -1) {
        weightingColText =
            map.getAttributeTable().getColumnName(static_cast<size_t>(m_weightedMeasureCol));
    }

    auto addColumn = [&newColumns, tulipBins = this->m_tulipBins, radiusType = this->m_radiusType](
                         const std::string &column, double radius, bool selectionOnly,
                         const std::optional<std::string> &routeWeightColName = std::nullopt,
                         const std::optional<std::string> &weightCol1Name = std::nullopt,
                         const std::optional<std::string> &weightCol2Name = std::nullopt,
                         const std::optional<int> &leafRef = std::nullopt) {
        newColumns.push_back(getFormattedColumn( //
            column, tulipBins, radiusType, radius, selectionOnly, routeWeightColName,
            weightCol1Name, weightCol2Name, leafRef));
    };
    for (auto radius : radii) {
        if (!m_forceLegacyColumnOrder) {
            // EF routeweight *
            if (m_routeweightCol != -1) {
                addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), routeweightColText);
                if (m_selSet.has_value() && m_recordSelLeafs) {
                    for (auto ref : *m_selSet) {
                        addColumn(Column::LEAF, radius, m_selSet.has_value(), routeweightColText,
                                  std::nullopt, std::nullopt, std::make_optional(ref));
                    }
                }
                if (m_weightedMeasureCol != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), routeweightColText,
                              weightingColText);
                }
                // EFEF*
                if (m_weightedMeasureCol2 != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), routeweightColText,
                              weightingColText, weightingColText2);
                }
                //*EFEF
            }
            //*EF routeweight
            else { // Normal run // TV
                addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value());
                if (m_selSet.has_value() && m_recordSelLeafs) {
                    for (auto ref : *m_selSet) {
                        addColumn(Column::LEAF, radius, m_selSet.has_value(), std::nullopt,
                                  std::nullopt, std::nullopt, std::make_optional(ref));
                    }
                }
                if (m_weightedMeasureCol != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), std::nullopt,
                              weightingColText);
                }
                // EFEF*
                if (m_weightedMeasureCol2 != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), std::nullopt,
                              weightingColText, weightingColText2);
                }
                //*EFEF
            }
        } else {
            // EF routeweight *
            if (m_routeweightCol != -1) {
                addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), routeweightColText);
                if (m_selSet.has_value() && m_recordSelLeafs) {
                    for (auto ref : *m_selSet) {
                        addColumn(Column::LEAF, radius, m_selSet.has_value(), routeweightColText,
                                  std::nullopt, std::nullopt, std::make_optional(ref));
                    }
                }
                if (m_weightedMeasureCol != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), routeweightColText,
                              weightingColText);
                }
                // EFEF*
                if (m_weightedMeasureCol2 != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), routeweightColText,
                              weightingColText, weightingColText2);
                }
                //*EFEF
            }
            //*EF routeweight
            else { // Normal run // TV
                addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value());
                if (m_selSet.has_value() && m_recordSelLeafs) {
                    for (auto ref : *m_selSet) {
                        addColumn(Column::LEAF, radius, m_selSet.has_value(), std::nullopt,
                                  std::nullopt, std::nullopt, std::make_optional(ref));
                    }
                }
                if (m_weightedMeasureCol != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), std::nullopt,
                              weightingColText);
                }
                // EFEF*
                if (m_weightedMeasureCol2 != -1) {
                    addColumn(Column::LEAF_CHOICE, radius, m_selSet.has_value(), std::nullopt,
                              weightingColText, weightingColText2);
                }
                //*EFEF
            }
        }
    }
    return newColumns;
}

AnalysisResult SegmentTulipLeafChoice::run(Communicator *comm, ShapeGraph &map, bool) {

    AnalysisResult result;
    if (map.getMapType() != ShapeMap::SEGMENTMAP) {
        return result;
    }

    if (m_recordSelLeafs && m_selSet.has_value() && m_selSet->size() > 50) {
        if (comm)
            comm->logError("Can not record leafs for more than 50 selected origins");
        return result;
    }

    // TODO: Understand what these parameters do. They were never truly provided in the original
    // function
    int weightingCol2 = m_weightedMeasureCol2;
    int routeweightCol = m_routeweightCol;
    bool interactive = m_interactive;

    AttributeTable &attributes = map.getAttributeTable();

    int processedRows = 0;

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
    bool radiusN = false;
    std::vector<double> radiusUnconverted;
    for (auto radius : m_radiusSet) {
        if (radius == -1.0) {
            radiusN = true;
        } else {
            radiusUnconverted.push_back(radius);
        }
    }
    if (radiusN) {
        radiusUnconverted.push_back(-1.0);
    }

    // retrieve weighted col data, as this may well be overwritten in the new analysis:
    std::vector<float> weights;
    std::vector<float> routeweights; // EF
    std::string weightingColText;

    int tulipBins = m_tulipBins;

    if (m_weightedMeasureCol != -1) {
        weightingColText = attributes.getColumnName(static_cast<size_t>(m_weightedMeasureCol));
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights.push_back(map.getAttributeRowFromShapeIndex(i).getValue(
                static_cast<size_t>(m_weightedMeasureCol)));
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights.push_back(1.0f);
        }
    }
    // EF routeweight*
    std::string routeweightColText;
    if (routeweightCol != -1) {
        // we normalise the column values between 0 and 1 and reverse it so that high values can be
        // treated as a 'low cost' - similar to the angular cost
        double maxValue = attributes.getColumn(static_cast<size_t>(routeweightCol)).getStats().max;
        routeweightColText = attributes.getColumnName(static_cast<size_t>(routeweightCol));
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            routeweights.push_back(
                static_cast<float>(1.0 - (map.getAttributeRowFromShapeIndex(i).getValue(
                                              static_cast<size_t>(routeweightCol)) /
                                          maxValue))); // scale and revert!
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
    std::string weightingColText2;
    if (weightingCol2 != -1) {
        weightingColText2 = attributes.getColumnName(static_cast<size_t>(weightingCol2));
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights2.push_back(
                map.getAttributeRowFromShapeIndex(i).getValue(static_cast<size_t>(weightingCol2)));
        }
    } else { // Normal run // TV
        for (size_t i = 0; i < map.getConnections().size(); i++) {
            weights2.push_back(1.0f);
        }
    }
    //*EFEF

    auto newColumns = getRequiredColumns(map, radiusUnconverted);
    for (auto &col : newColumns) {
        attributes.insertOrResetColumn(col);
        result.addAttribute(col);
    }

    std::string tulipText = std::string("T") + dXstring::formatString(tulipBins, "%d");

    auto nradii = radiusUnconverted.size();

    std::vector<size_t> choiceCol, wChoiceCol, wChoiceCol2;
    std::unordered_map<int, std::vector<size_t>> leafCol;
    // then look them up! eek....
    size_t radiusIdx = 0;
    for (auto radius : radiusUnconverted) {
        std::string radiusText = makeRadiusText(m_radiusType, radius);
        // EF routeweight *
        if (routeweightCol != -1) {
            choiceCol.push_back(getFormattedColumnIdx( //
                attributes, Column::LEAF_CHOICE, m_tulipBins, m_radiusType, radius,
                m_selSet.has_value(), routeweightColText));

            if (m_selSet.has_value() && m_recordSelLeafs) {
                for (auto ref : *m_selSet) {
                    auto lc = leafCol.find(ref);
                    if (lc == leafCol.end()) {
                        lc = leafCol.insert({ref, std::vector<size_t>(nradii)}).first;
                    }
                    lc->second[radiusIdx] = getFormattedColumnIdx( //
                        attributes, Column::LEAF, m_tulipBins, m_radiusType, radius,
                        m_selSet.has_value(), std::nullopt, std::nullopt, routeweightColText,
                        std::make_optional(ref));
                }
            }
            if (m_weightedMeasureCol != -1) {
                wChoiceCol.push_back(getFormattedColumnIdx( //
                    attributes, Column::LEAF_CHOICE, m_tulipBins, m_radiusType, radius,
                    m_selSet.has_value(), routeweightColText, weightingColText));
            }
            // EFEF*
            if (weightingCol2 != -1) {
                wChoiceCol2.push_back(getFormattedColumnIdx( //
                    attributes, Column::LEAF_CHOICE, m_tulipBins, m_radiusType, radius,
                    m_selSet.has_value(), routeweightColText, weightingColText, weightingColText2));
            }
            //*EFEF
        }
        //* EF routeweight
        else { // Normal run // TV

            choiceCol.push_back(getFormattedColumnIdx( //
                attributes, Column::LEAF_CHOICE, m_tulipBins, m_radiusType, radius,
                m_selSet.has_value()));
            if (m_selSet.has_value() && m_recordSelLeafs) {
                for (auto ref : *m_selSet) {
                    auto lc = leafCol.find(ref);
                    if (lc == leafCol.end()) {
                        lc = leafCol.insert({ref, std::vector<size_t>(nradii)}).first;
                    }
                    lc->second[radiusIdx] = getFormattedColumnIdx( //
                        attributes, Column::LEAF, m_tulipBins, m_radiusType, radius,
                        m_selSet.has_value(), std::nullopt, std::nullopt, std::nullopt,
                        std::make_optional(ref));
                }
            }
            if (m_weightedMeasureCol != -1) {
                wChoiceCol.push_back(getFormattedColumnIdx( //
                    attributes, Column::LEAF_CHOICE, m_tulipBins, m_radiusType, radius,
                    m_selSet.has_value(), std::nullopt, weightingColText));
            }
            // EFEF*
            if (weightingCol2 != -1) {
                wChoiceCol2.push_back(getFormattedColumnIdx( //
                    attributes, Column::LEAF_CHOICE, m_tulipBins, m_radiusType, radius,
                    m_selSet.has_value(), std::nullopt, weightingColText, weightingColText2));
            }
            //*EFEF
        }
        ++radiusIdx;
    }

    tulipBins /= 2; // <- actually use semicircle of tulip bins
    tulipBins += 1;

    std::vector<std::vector<SegmentData>> bins(static_cast<size_t>(tulipBins));

    auto nconnections = map.getConnections().size();

    // TODO: Replace these with STL
    AnalysisInfo ***audittrail;
    unsigned int **uncovered;
    audittrail = new AnalysisInfo **[nconnections];
    uncovered = new unsigned int *[nconnections];
    for (size_t i = 0; i < nconnections; i++) {
        audittrail[i] = new AnalysisInfo *[nradii];
        for (size_t j = 0; j < nradii; j++) {
            audittrail[i][j] = new AnalysisInfo[2];
        }
        uncovered[i] = new unsigned int[2];
    }
    std::vector<double> radius;

    for (auto uradius : radiusUnconverted) {
        if (m_radiusType == RadiusType::ANGULAR && uradius != -1) {
            radius.push_back(floor(uradius * tulipBins * 0.5));
        } else {
            radius.push_back(uradius);
        }
    }
    // entered once for each segment
    std::vector<float> lengths;
    auto lengthCol = attributes.getColumnIndex("Segment Length");
    for (size_t i = 0; i < nconnections; i++) {
        AttributeRow &row = map.getAttributeRowFromShapeIndex(i);
        lengths.push_back(row.getValue(lengthCol));
    }

    int radiusmask = 0;
    for (size_t i = 0; i < nradii; i++) {
        radiusmask |= (1 << i);
    }

    for (size_t cursor = 0; cursor < nconnections; cursor++) {
        auto &shapeRef = map.getShapeRefFromIndex(cursor)->first;
        AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(shapeRef));

        if (m_selSet.has_value() && m_selSet->find(shapeRef) == m_selSet->end()) {
            continue;
        }

        for (int k = 0; k < tulipBins; k++) {
            bins[static_cast<size_t>(k)].clear();
        }
        for (size_t j = 0; j < nconnections; j++) {
            for (int dir = 0; dir < 2; dir++) {
                for (size_t k = 0; k < nradii; k++) {
                    audittrail[j][k][dir].clearLine();
                }
                uncovered[j][dir] = static_cast<unsigned int>(radiusmask);
            }
        }

        double rootseglength = row.getValue(lengthCol);
        double rootweight = (m_weightedMeasureCol != -1) ? weights[cursor] : 0.0;

        // setup: direction 0 (both ways), segment i, previous -1, segdepth (step depth) 0,
        // metricdepth 0.5 * rootseglength, bin 0
        SegmentData segmentData(0, static_cast<int>(cursor), SegmentRef(), 0,
                                static_cast<float>(0.5 * rootseglength),
                                static_cast<unsigned int>(radiusmask));
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
                if (currentbin == static_cast<size_t>(tulipBins)) {
                    currentbin = 0;
                }
            }
            SegmentData lineindex = bins[currentbin].back();
            bins[currentbin].pop_back();
            //
            opencount--;

            int ref = lineindex.ref;
            int dir = (lineindex.dir == 1) ? 0 : 1;
            auto coverage = lineindex.coverage & uncovered[ref][dir];
            if (coverage != 0) {
                int rbin = 0;
                int rbinbase;
                if (lineindex.previous.ref != -1) {
                    uncovered[ref][dir] &= ~coverage;
                    while (((coverage >> rbin) & 0x1) == 0)
                        rbin++;
                    rbinbase = rbin;
                    while (rbin < static_cast<int>(nradii)) {
                        if (((coverage >> rbin) & 0x1) == 1) {
                            auto &adtrCurrent = audittrail[ref][rbin][dir];
                            adtrCurrent.depth = depthlevel;
                            adtrCurrent.previous = lineindex.previous;
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
                Connector &line = map.getConnections()[static_cast<size_t>(ref)];
                float seglength;
                int extradepth;
                if (lineindex.dir != -1) {
                    for (auto &segconn : line.forwardSegconns) {
                        rbin = rbinbase;
                        SegmentRef conn = segconn.first;
                        if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
                            // EF routeweight*
                            if (routeweightCol !=
                                -1) { // EF here we do the weighting of the angular cost by the
                                      // weight of the next segment
                                // note that the content of the routeweights array is scaled between
                                // 0 and 1 and is reversed such that: = 1.0-(attributes.getValue(i,
                                // routeweight_col)/max_value)
                                extradepth = static_cast<int>(
                                    floor(segconn.second * static_cast<float>(tulipBins) * 0.5 *
                                          routeweights[static_cast<size_t>(conn.ref)]));
                            }
                            //*EF routeweight
                            else {
                                extradepth = static_cast<int>(
                                    floor(segconn.second * static_cast<float>(tulipBins) * 0.5));
                            }
                            seglength = lengths[static_cast<size_t>(conn.ref)];
                            switch (m_radiusType) {
                            case RadiusType::ANGULAR:
                                while (rbin != static_cast<int>(nradii) &&
                                       radius[static_cast<size_t>(rbin)] != -1 &&
                                       depthlevel + extradepth >
                                           static_cast<int>(radius[static_cast<size_t>(rbin)])) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::METRIC:
                                while (rbin != static_cast<int>(nradii) &&
                                       radius[static_cast<size_t>(rbin)] != -1 &&
                                       lineindex.metricdepth + seglength * 0.5 >
                                           radius[static_cast<size_t>(rbin)]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::TOPOLOGICAL:
                                if (rbin != static_cast<int>(nradii) &&
                                    radius[static_cast<size_t>(rbin)] != -1 &&
                                    lineindex.segdepth >=
                                        static_cast<int>(radius[static_cast<size_t>(rbin)])) {
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
                                size_t bin = (currentbin + static_cast<size_t>(tulipBins) +
                                              static_cast<size_t>(extradepth)) %
                                             static_cast<size_t>(tulipBins);
                                genlib::insert_sorted(bins[bin], sd);
                                opencount++;
                            }
                        }
                    }
                }
                if (lineindex.dir != 1) {
                    for (auto &segconn : line.backSegconns) {
                        rbin = rbinbase;
                        SegmentRef conn = segconn.first;
                        if ((uncovered[conn.ref][(conn.dir == 1 ? 0 : 1)] & coverage) != 0) {
                            // EF routeweight*
                            if (routeweightCol !=
                                -1) { // EF here we do the weighting of the angular cost by the
                                      // weight of the next segment
                                // note that the content of the routeweights array is scaled between
                                // 0 and 1 and is reversed such that: = 1.0-(attributes.getValue(i,
                                // routeweight_col)/max_value)
                                extradepth = static_cast<int>(
                                    floor(segconn.second * static_cast<float>(tulipBins) * 0.5 *
                                          routeweights[static_cast<size_t>(conn.ref)]));
                            }
                            //*EF routeweight
                            else {
                                extradepth = static_cast<int>(
                                    floor(segconn.second * static_cast<float>(tulipBins) * 0.5));
                            }
                            seglength = lengths[static_cast<size_t>(conn.ref)];
                            switch (m_radiusType) {
                            case RadiusType::ANGULAR:
                                while (rbin != static_cast<int>(nradii) &&
                                       radius[static_cast<size_t>(rbin)] != -1 &&
                                       depthlevel + extradepth >
                                           static_cast<int>(radius[static_cast<size_t>(rbin)])) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::METRIC:
                                while (rbin != static_cast<int>(nradii) &&
                                       radius[static_cast<size_t>(rbin)] != -1 &&
                                       lineindex.metricdepth + seglength * 0.5 >
                                           radius[static_cast<size_t>(rbin)]) {
                                    rbin++;
                                }
                                break;
                            case RadiusType::TOPOLOGICAL:
                                if (rbin != static_cast<int>(nradii) &&
                                    radius[static_cast<size_t>(rbin)] != -1 &&
                                    lineindex.segdepth >=
                                        static_cast<int>(radius[static_cast<size_t>(rbin)])) {
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
                                size_t bin = (currentbin + static_cast<size_t>(tulipBins) +
                                              static_cast<size_t>(extradepth)) %
                                             static_cast<size_t>(tulipBins);
                                genlib::insert_sorted(bins[bin], sd);
                                opencount++;
                            }
                        }
                    }
                }
            }
        }
        // set the attributes for this node:
        for (size_t k = 0; k < nradii; k++) {
            size_t j;
            for (j = 0; j < nconnections; j++) {
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

                    // Leaf choice: This is where the issue begins. Only process leaf nodes
                    if (audittrail[j][k][dir].leaf) {
                        // note, graph may be directed (e.g., for one way streets), so both ways
                        // must be included from now on:
                        SegmentRef here = SegmentRef(dir == 0 ? 1 : -1, static_cast<int>(j));
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
                                // std::cout << here.ref << " (" << choicecount << ") ";
                                // each node has the existing choicecount and choiceweight from
                                // previously encountered nodes added to it
                                auto &adt = audittrail[here.ref][k][heredir];
                                adt.choice += choicecount;
                                // nb, weighted values calculated anyway to save time on 'if'
                                adt.weightedChoice += choiceweight;
                                // EFEF*
                                adt.weightedChoice2 += choiceweight2;
                                //*EFEF
                                // if the node hasn't been encountered before, the choicecount and
                                // choiceweight is incremented for all remaining nodes to be
                                // encountered on the backwards route from it
                                if (!adt.choicecovered) {
                                    // this node has not been encountered before: this adds the
                                    // choicecount and weight for this node, and flags it as visited
                                    choicecount++;
                                    choiceweight +=
                                        weights[static_cast<size_t>(here.ref)] * rootweight;
                                    // EFEF*
                                    choiceweight2 += weights2[static_cast<size_t>(here.ref)] *
                                                     rootweight; // rootweight!
                                    //*EFEF

                                    adt.choicecovered = true;
                                    // note, for weighted choice, the start and end points have
                                    // choice added to them:
                                    if (m_weightedMeasureCol != -1) {
                                        adt.weightedChoice +=
                                            (weights[static_cast<size_t>(here.ref)] * rootweight) /
                                            2.0;
                                        // EFEF*
                                        if (weightingCol2 != -1) {
                                            adt.weightedChoice2 +=
                                                (weights2[static_cast<size_t>(here.ref)] *
                                                 rootweight) /
                                                2.0; // rootweight!
                                        }
                                        //*EFEF
                                    }
                                }
                                here = adt.previous;
                            }
                            // note, for weighted choice, the start and end points have choice added
                            // to them: (this is the summed weight for all starting nodes
                            // encountered in this path)
                            if (m_weightedMeasureCol != -1) {
                                audittrail[here.ref][k][(here.dir == 1) ? 0 : 1].weightedChoice +=
                                    choiceweight / 2.0;
                                // EFEF*
                                if (weightingCol2 != -1) {
                                    audittrail[here.ref][k][(here.dir == 1) ? 0 : 1]
                                        .weightedChoice2 += choiceweight2 / 2.0;
                                }
                                //*EFEF
                            }
                        }
                    }
                }
            }
        }

        if (m_recordSelLeafs && m_selSet.has_value() &&
            m_selSet->find(shapeRef) != m_selSet->end()) {
            for (size_t j = 0; j < nconnections; j++) {
                for (size_t k = 0; k < nradii; k++) {
                    auto &adtr = audittrail[j][k];
                    float leafVal = 0; // not a leaf
                    if (adtr[0].leaf && adtr[1].leaf) {
                        // leaf on both directions
                        leafVal = 3;
                    } else if (adtr[0].leaf) {
                        // leaf forward
                        leafVal = 1;
                    } else if (adtr[1].leaf) {
                        // leaf backwards
                        leafVal = 2;
                    }
                    auto &connShapeRef = map.getShapeRefFromIndex(j)->first;
                    AttributeRow &connRow =
                        map.getAttributeTable().getRow(AttributeKey(connShapeRef));
                    connRow.setValue(leafCol[shapeRef][k], leafVal);
                }
            }
        }

        //
        processedRows++;
        //
        if (comm) {
            if (qtimer(atime, 500)) {
                if (comm->IsCancelled()) {
                    // interactive is usual Depthmap: throw an exception if cancelled
                    if (interactive) {
                        for (size_t i = 0; i < nconnections; i++) {
                            for (size_t j = 0; j < static_cast<size_t>(nradii); j++) {
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
    for (size_t cursor = 0; cursor < nconnections; cursor++) {
        auto &shapeRef = map.getShapeRefFromIndex(cursor)->first;
        AttributeRow &row = map.getAttributeTable().getRow(AttributeKey(shapeRef));

        for (size_t r = 0; r < nradii; r++) {
            auto &adtr = audittrail[cursor][r];
            // according to Eva's correction, total choice and total weighted choice
            // should already have been accumulated by radius at this stage
            double totalChoice = adtr[0].choice + adtr[1].choice;
            double totalWeightedChoice = adtr[0].weightedChoice + adtr[1].weightedChoice;
            // EFEF*
            double totalWeightedChoice2 = adtr[0].weightedChoice2 + adtr[1].weightedChoice2;
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
            row.setValue(choiceCol[r], static_cast<float>(totalChoice));
            if (m_weightedMeasureCol != -1) {
                row.setValue(wChoiceCol[r], static_cast<float>(totalWeightedChoice));
                // EFEF*
                if (weightingCol2 != -1) {
                    row.setValue(wChoiceCol2[r], static_cast<float>(totalWeightedChoice2));
                }
                //*EFEF
            }
        }
    }

    for (size_t i = 0; i < nconnections; i++) {
        for (size_t j = 0; j < nradii; j++) {
            delete[] audittrail[i][j];
        }
        delete[] audittrail[i];
        delete[] uncovered[i];
    }
    delete[] audittrail;
    delete[] uncovered;

    result.completed = processedRows > 0;

    return result;
}
