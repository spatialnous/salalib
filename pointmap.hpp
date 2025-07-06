// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "attributemap.hpp"
#include "layermanagerimpl.hpp"
#include "point.hpp"
#include "shapemap.hpp"
#include "sparksieve2.hpp"

#include "genlib/comm.hpp"
#include "genlib/exceptions.hpp"
#include "genlib/simplematrix.hpp"

#include <set>
#include <vector>

namespace sala {
    enum PointMapExceptionType { NO_ISOVIST_ANALYSIS };
    class PointMapException : public genlib::RuntimeException {
      private:
        PointMapExceptionType m_errorType;

        [[maybe_unused]] unsigned _padding0 : 4 * 8;

      public:
        PointMapException(PointMapExceptionType errorType, std::string message)
            : genlib::RuntimeException(std::move(message)), m_errorType(errorType), _padding0(0) {}
        PointMapExceptionType getErrorType() const { return m_errorType; }
    };
} // namespace sala

class PointMap : public AttributeMap {

  public: // members
    bool hasIsovistAnalysis() {
        for (size_t j = 0; j < m_cols; j++) {
            for (size_t k = 0; k < m_rows; k++) {
                // check if occdistance of any pixel's bin is set, meaning that
                // the isovist analysis was done
                for (int b = 0; b < 32; b++) {
                    if (m_points(k, j).m_node && m_points(k, j).m_node->occdistance(b) > 0) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

  protected:
    genlib::ColumnMatrix<Point> m_points; // will contain the graph reference when created
    std::vector<PixelRefPair> m_mergeLines;
    double m_spacing;
    Point2f m_offset;
    Point2f m_bottomLeft;
    int m_filledPointCount;
    int m_undocounter;
    bool m_initialised;
    bool m_blockedlines;
    bool m_processed;
    bool m_boundarygraph;

  private:
    [[maybe_unused]] unsigned _padding0 : 4 * 8;

  public: // known columns
    struct Column {
        inline static const std::string                  //
            CONNECTIVITY = "Connectivity",               //
            POINT_FIRST_MOMENT = "Point First Moment",   //
            POINT_SECOND_MOMENT = "Point Second Moment"; //
    };

  public: // ctors
    PointMap(Region4f region, const std::string &name = std::string("VGA Map"));
    ~PointMap() override {}
    void copyData(const PointMap &sourcemap, bool copypoints = false, bool copyattributes = false);
    void copy(const PointMap &sourcemap, bool copypoints = false, bool copyattributes = false);

    void resetBlockedLines() { m_blockedlines = false; }

    PointMap(PointMap &&other)
        : AttributeMap(std::move(other.m_name), std::move(other.m_attributes),
                       std::move(other.m_attribHandle), std::move(other.m_layers)),
          m_points(std::move(other.m_points)), m_mergeLines(), m_spacing(), m_offset(),
          m_bottomLeft(), m_filledPointCount(), m_undocounter(), m_initialised(), m_blockedlines(),
          m_processed(), m_boundarygraph(), _padding0(0) {
        m_region = std::move(other.m_region);
        copyData(other);
    }
    PointMap &operator=(PointMap &&other) {
        m_region = std::move(other.m_region);
        m_points = std::move(other.m_points);
        m_attributes = std::move(other.m_attributes);
        m_attribHandle = std::move(other.m_attribHandle);
        m_layers = std::move(other.m_layers);
        copyData(other);
        return *this;
    }
    PointMap(const PointMap &) = delete;
    PointMap &operator=(const PointMap &) = delete;

  public: // methods
    void communicate(time_t &atime, Communicator *comm, size_t record);
    // constrain is constrain to existing rows / cols
    PixelRef pixelate(const Point2f &p, bool constrain = true, int scalefactor = 1) const override;
    Point2f depixelate(const PixelRef &p, double scalefactor = 1.0) const; // Inlined below
    Region4f regionate(const PixelRef &p, double border) const;            // Inlined below
    void addPointsInRegionToSet(const Region4f &r, std::set<PixelRef> &selSet);
    std::set<PixelRef> getPointsInRegion(const Region4f &r) const;
    bool setGrid(double spacing, const Point2f &offset = Point2f());
    std::vector<std::pair<PixelRef, PixelRef>> getMergedPixelPairs() {
        // unnecessary converter until the m_merge_lines variable is
        // replaced with a std container
        std::vector<std::pair<PixelRef, PixelRef>> mergedPixelPairs;
        for (size_t i = 0; i < m_mergeLines.size(); i++) {
            mergedPixelPairs.push_back(std::make_pair(m_mergeLines[i].a, m_mergeLines[i].b));
        }
        return mergedPixelPairs;
    }
    const std::vector<PixelRefPair> &getMergeLines() const { return m_mergeLines; }

    bool isProcessed() const { return m_processed; }
    void fillLine(const Line4f &li);
    bool blockLines(std::vector<Line4f> &lines);
    void blockLine(const Line4f &li);
    void unblockLines(bool clearblockedflag = true);
    bool fillPoint(const Point2f &p, bool add = true); // use add = false for remove point
    // bool blockPoint(const Point2f& p, bool add = true); // no longer used
    //
    bool makePoints(const Point2f &seed, int fillType,
                    Communicator *comm = nullptr); // Point2f non-reference deliberate
    bool clearAllPoints();                         // Clear *selected* points
    bool clearPointsInRange(PixelRef bl, PixelRef tr,
                            std::set<int> &selSet); // Clear *selected* points
    bool undoPoints();
    bool canUndo() const { return !m_processed && m_undocounter != 0; }
    void outputPoints(std::ostream &stream, char delim);
    void outputMergeLines(std::ostream &stream, char delim);
    size_t tagState(bool settag);
    bool sparkGraph2(Communicator *comm, bool boundarygraph, double maxdist);
    bool unmake(bool removeLinks);
    bool sparkPixel2(PixelRef curs, int make, double maxdist = -1.0);
    bool sieve2(sparkSieve2 &sieve, std::vector<PixelRef> &addlist, int q, int depth,
                PixelRef curs);
    // bool makeGraph( Graph& graph, int optimization_level = 0, Communicator *comm = NULL);
    //
    bool binDisplay(Communicator *, std::set<int> &selSet);
    bool mergePoints(const Point2f &p, Region4f &firstPointsBounds, std::set<int> &firstPoints);
    bool unmergePoints(std::set<int> &firstPoints);
    bool unmergePixel(PixelRef a);
    bool mergePixels(PixelRef a, PixelRef b);
    void mergeFromShapeMap(const ShapeMap &shapemap);
    bool isPixelMerged(const PixelRef &a);

    void outputSummary(std::ostream &myout, char delimiter = '\t');
    void outputMif(std::ostream &miffile, std::ostream &midfile);
    void outputNet(std::ostream &netfile);
    void outputConnections(std::ostream &myout);
    void outputBinSummaries(std::ostream &myout);

    const Point &getPoint(const PixelRef &p) const {
        return m_points(static_cast<size_t>(p.y), static_cast<size_t>(p.x));
    }
    Point &getPoint(const PixelRef &p) {
        return m_points(static_cast<size_t>(p.y), static_cast<size_t>(p.x));
    }
    genlib::BaseMatrix<Point> &getPoints() { return m_points; }
    const genlib::BaseMatrix<Point> &getPoints() const { return m_points; }
    const int &pointState(const PixelRef &p) const {
        return m_points(static_cast<size_t>(p.y), static_cast<size_t>(p.x)).m_state;
    }
    // to be phased out
    bool blockedAdjacent(const PixelRef p) const;

    int getFilledPointCount() const { return m_filledPointCount; }

    void requireIsovistAnalysis() {
        if (!hasIsovistAnalysis()) {
            throw sala::PointMapException(sala::PointMapExceptionType::NO_ISOVIST_ANALYSIS,
                                          "Current pointmap does not contain isovist analysis");
        }
    }

    bool readMetadata(std::istream &stream);
    bool readPointsAndAttributes(std::istream &stream);
    std::tuple<bool, int> read(std::istream &stream);

    bool writeMetadata(std::ostream &stream) const;
    bool writePointsAndAttributes(std::ostream &stream) const;
    bool write(std::ostream &stream, int displayedAttribute = -1) const;

  protected:
    int expand(const PixelRef p1, const PixelRef p2, PixelRefVector &list, int filltype);
    //
    // void walk( PixelRef& start, int steps, Graph& graph,
    //           int parity, int dominant_axis, const int grad_pair[] );

  public:
    PixelRefVector getLayerPixels(int layer);

    double getLocationValue(const Point2f &point, std::optional<size_t> columnIdx);
    //
    // Screen functionality
  public:
    enum { VIEW_ATTRIBUTES, VIEW_MERGED, VIEW_LAYERS, VIEW_AGENTS };

    //
    double getSpacing() const { return m_spacing; }

    // this is an odd helper function, value in range 0 to 1
    PixelRef pickPixel(double value) const;

    void addGridConnections(); // adds grid connections where graph does not include them
    void outputConnectionsAsCSV(std::ostream &myout, std::string delim = ",");
    void outputLinksAsCSV(std::ostream &myout, std::string delim = ",");
};

// inlined to make thread safe

inline Point2f PointMap::depixelate(const PixelRef &p, double scalefactor) const {
    return Point2f(m_bottomLeft.x + m_spacing * scalefactor * static_cast<double>(p.x),
                   m_bottomLeft.y + m_spacing * scalefactor * static_cast<double>(p.y));
}

inline Region4f PointMap::regionate(const PixelRef &p, double border) const {
    return Region4f(
        Point2f(m_bottomLeft.x + m_spacing * (static_cast<double>(p.x) - 0.5 - border),
                m_bottomLeft.y + m_spacing * (static_cast<double>(p.y) - 0.5 - border)),
        Point2f(m_bottomLeft.x + m_spacing * (static_cast<double>(p.x) + 0.5 + border),
                m_bottomLeft.y + m_spacing * (static_cast<double>(p.y) + 0.5 + border)));
}

/////////////////////////////////////////////////////////////////////////////////////

// true grads are also similar to generated grads...
// this scruffy helper function converts a true grad to a bin:

// (now corrected as of 2.1008r!)

inline int whichbin(const Point2f &grad) {
    int bin = 0;
    double ratio;

    // This is only for true gradients...
    //    ...see below for calculated gradients
    //
    // Octant:
    //       +     -
    //    - \ 8 | 8 / +
    //      16\ | / 0
    //      ---- ----
    //      16/ | \32
    //    + /24 | 24\ -
    //      -      +

    if (fabs(grad.y) > fabs(grad.x)) {
        bin = 1; // temporary: label y priority
    }

    if (bin == 0) {
        ratio = fabs(grad.y) / fabs(grad.x);

        // now actual bin number
        if (grad.x > 0.0) {
            if (grad.y >= 0.0) {
                bin = 0;
            } else {
                bin = -32;
            }
        } else {
            if (grad.y >= 0.0) {
                bin = -16;
            } else {
                bin = 16;
            }
        }
    } else {
        ratio = fabs(grad.x) / fabs(grad.y);

        // now actual bin number
        if (grad.y > 0.0) {
            if (grad.x >= 0.0) {
                bin = -8;
            } else {
                bin = 8;
            }
        } else {
            if (grad.x >= 0.0) {
                bin = 24;
            } else {
                bin = -24;
            }
        }
    }

    if (ratio < 1e-12) {
        // nop
    } else if (ratio < 0.2679491924311227) { // < 15 degrees
        bin += 1;
    } else if (ratio < 0.5773502691896257) { // < 30 degrees
        bin += 2;
    } else if (ratio < 1.0 - 1e-12) { // < 45 degrees
        bin += 3;
    } else {
        bin += 4;
    }

    if (bin < 0) {
        bin = -bin;
    }
    // this is necessary:
    bin = bin % 32;

    return bin;
}

/////////////////////////////////

// Another helper to write down the q-octant from any bin, in shifted format
// note that sieve2 has been used to get the precise required q-octant for the bin

inline int processoctant(int bin) {
    int q = -1;
    switch (bin) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        q = 1;
        break;
    case 5:
    case 6:
    case 7:
        q = 7;
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        q = 6;
        break;
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
        q = 0;
        break;
    case 17:
    case 18:
    case 19:
    case 20:
        q = 2;
        break;
    case 21:
    case 22:
    case 23:
        q = 4;
        break;
    case 24:
    case 25:
    case 26:
    case 27:
        q = 5;
        break;
    case 28:
    case 29:
    case 30:
    case 31:
        q = 3;
        break;
    default:
        throw std::runtime_error("bin can only be between 0 and 31");
    }

    return (1 << q);
}

// ...but in order to determine what *needs* processing, we need this octant:

inline int flagoctant(int bin) {
    int q = 0;

    // have to use two q octants if you are on diagonals or axes...
    switch (bin) {
    case 0:
        q |= 1 << 1;
        q |= 1 << 3;
        break;
    case 1:
    case 2:
    case 3:
        q |= 1 << 1;
        break;
    case 4:
        q |= 1 << 1;
        q |= 1 << 7;
        break;
    case 5:
    case 6:
    case 7:
        q |= 1 << 7;
        break;
    case 8:
        q |= 1 << 7;
        q |= 1 << 6;
        break;
    case 9:
    case 10:
    case 11:
        q = 1 << 6;
        break;
    case 12:
        q |= 1 << 6;
        q |= 1 << 0;
        break;
    case 13:
    case 14:
    case 15:
        q |= 1 << 0;
        break;
    case 16:
        q |= 1 << 0;
        q |= 1 << 2;
        break;
    case 17:
    case 18:
    case 19:
        q |= 1 << 2;
        break;
    case 20:
        q |= 1 << 2;
        q |= 1 << 4;
        break;
    case 21:
    case 22:
    case 23:
        q |= 1 << 4;
        break;
    case 24:
        q |= 1 << 4;
        q |= 1 << 5;
        break;
    case 25:
    case 26:
    case 27:
        q |= 1 << 5;
        break;
    case 28:
        q |= 1 << 5;
        q |= 1 << 3;
        break;
    case 29:
    case 30:
    case 31:
        q |= 1 << 3;
        break;
    }

    return q;
}

// Another helper, this time to write down the q-octant for the bin opposing you

inline int q_opposite(int bin) {
    int opposingBin = (16 + bin) % 32;

    /*
     *       \ 6 | 7 /
     *      0 \ | / 1
     *      - -   - -
     *      2 / | \ 3
     *      / 4 | 5 \
     */

    return flagoctant(opposingBin);
}
