// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tidylines.hpp"

#include "tolerances.hpp"

// helper -- a little class to tidy up a set of lines

void TidyLines::tidy(std::vector<Line4f> &lines, const Region4f &region) {
    m_region = region;
    double maxdim = std::max(m_region.width(), m_region.height());

    // simple first pass -- remove very short lines
    lines.erase(std::remove_if(
                    lines.begin(), lines.end(),
                    [maxdim](const Line4f &line) { return line.length() < maxdim * TOLERANCE_B; }),
                lines.end());

    // now load up m_lines...
    initLines(static_cast<int>(lines.size()), m_region.bottomLeft, m_region.topRight);
    for (auto &line : lines) {
        addLine(line);
    }
    sortPixelLines();

    std::vector<int> removelist;
    for (size_t i = 0; i < lines.size(); i++) {
        // n.b., as m_lines have just been made, note that what's in m_lines matches
        // whats in lines we will use this later!
        m_test++;
        m_lines[static_cast<int>(i)].test = m_test;
        PixelRefVector list = pixelateLine(m_lines[static_cast<int>(i)].line);
        for (size_t a = 0; a < list.size(); a++) {
            auto pixelLines =
                m_pixelLines(static_cast<size_t>(list[a].y), static_cast<size_t>(list[a].x));
            for (int j : pixelLines) {
                auto uj = static_cast<size_t>(j);
                if (m_lines[j].test != m_test && j > static_cast<int>(i) &&
                    lines[i].Region4f::intersects(lines[uj], TOLERANCE_B * maxdim)) {
                    m_lines[j].test = m_test;
                    LineAxis axisI =
                        (lines[i].width() >= lines[i].height()) ? LineAxis::XAXIS : LineAxis::YAXIS;
                    LineAxis axisJ = (lines[uj].width() >= lines[uj].height()) ? LineAxis::XAXIS
                                                                               : LineAxis::YAXIS;
                    LineAxis axisReverse =
                        (axisI == LineAxis::XAXIS) ? LineAxis::YAXIS : LineAxis::XAXIS;
                    if (axisI == axisJ &&
                        fabs(lines[i].grad(axisReverse) - lines[uj].grad(axisReverse)) <
                            TOLERANCE_A &&
                        fabs(lines[i].constant(axisReverse) - lines[uj].constant(axisReverse)) <
                            (TOLERANCE_B * maxdim)) {
                        // check for overlap and merge
                        int parity = (axisI == LineAxis::XAXIS) ? 1 : lines[i].sign();
                        if ((lines[i].start()[axisI] * parity + TOLERANCE_B * maxdim) >
                                (lines[uj].start()[axisJ] * parity) &&
                            (lines[i].start()[axisI] * parity) <
                                (lines[uj].end()[axisJ] * parity + TOLERANCE_B * maxdim)) {
                            size_t end = ((lines[i].end()[axisI] * parity) >
                                          (lines[uj].end()[axisJ] * parity))
                                             ? i
                                             : uj;
                            lines[uj].bx() = lines[end].bx();
                            lines[uj].by() = lines[end].by();
                            removelist.push_back(static_cast<int>(i));
                            continue; // <- don't do this any more, we've zapped it and
                                      // replaced it with the later line
                        }
                        if ((lines[uj].start()[axisJ] * parity + TOLERANCE_B * maxdim) >
                                (lines[i].start()[axisI] * parity) &&
                            (lines[uj].start()[axisJ] * parity) <
                                (lines[i].end()[axisI] * parity + TOLERANCE_B * maxdim)) {
                            size_t end = ((lines[i].end()[axisI] * parity) >
                                          (lines[uj].end()[axisJ] * parity))
                                             ? i
                                             : uj;
                            lines[uj].ax() = lines[i].ax();
                            lines[uj].ay() = lines[i].ay();
                            lines[uj].bx() = lines[end].bx();
                            lines[uj].by() = lines[end].by();
                            removelist.push_back(static_cast<int>(i));
                            continue; // <- don't do this any more, we've zapped it and
                                      // replaced it with the later line
                        }
                    }
                }
            }
        }
    }

    // comes out sorted, remove duplicates just in case
    removelist.erase(std::unique(removelist.begin(), removelist.end()), removelist.end());

    for (auto iter = removelist.rbegin(); iter != removelist.rend(); ++iter)
        lines.erase(lines.begin() + *iter);
    removelist.clear(); // always clear this list, it's reused
}

void TidyLines::quicktidy(std::map<int, std::pair<Line4f, int>> &lines, const Region4f &region) {
    m_region = region;

    double avglen = 0.0;

    for (const auto &line : lines) {
        avglen += line.second.first.length();
    }
    avglen /= static_cast<double>(lines.size());

    double tolerance = avglen * 10e-6;

    auto iter = lines.begin(), end = lines.end();
    for (; iter != end;) {
        if (iter->second.first.length() < tolerance) {
            iter = lines.erase(iter);
        } else {
            ++iter;
        }
    }

    // now load up m_lines...
    initLines(static_cast<int>(lines.size()), m_region.bottomLeft, m_region.topRight);
    for (const auto &line : lines) {
        addLine(line.second.first);
    }
    sortPixelLines();

    // and chop duplicate lines:
    std::vector<int> removelist;
    int i = -1;
    for (const auto &line : lines) {
        i++;
        PixelRef start = pixelate(line.second.first.start());
        auto &pixelLines = m_pixelLines(static_cast<size_t>(start.y), static_cast<size_t>(start.x));
        for (int k : pixelLines) {
            if (k > static_cast<int>(i) &&
                m_lines[i].line.start().approxeq(m_lines[k].line.start(), tolerance)) {
                if (m_lines[i].line.end().approxeq(m_lines[k].line.end(), tolerance)) {
                    removelist.push_back(line.first);
                    break;
                }
            }
        }
    }
    for (int remove : removelist) {
        lines.erase(remove);
    }
    removelist.clear(); // always clear this list, it's reused}
}
