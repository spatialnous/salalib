// SPDX-FileCopyrightText: 2000-2010 University College London, Alasdair Turner
// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bsptree.hpp"
#include "salalib/genlib/exceptions.hpp"

#include <stack>

// Binary Space Partition

/* Takes a set of lines and creates a binary-space-partition tree by starting from a
 * root node, setting its left and right nodes and recursively doing the same process
 * over those. Through this process the set of lines is split in two (one set for each
 * left and right nodes) and those are split and passed again further down the recursion.
 * While the original implementation was actually recursive it was hitting the recursion
 * limit when the input was a large number of lines that fell on the same side (i.e. an
 * arc divided in 500 pieces). It has been refactored here to an iterative solution, where
 * the current node (left or right) is pushed to a stack along with the relevant set of lines.
 */

void BSPTree::make(Communicator *communicator, time_t atime, const std::vector<Line4f> &lines,
                   BSPNode *root) {

    typedef std::pair<std::vector<Line4f>, std::vector<Line4f>> LineVecPair;

    std::stack<BSPNode *> nodeStack;
    std::stack<LineVecPair> lineStack;

    nodeStack.push(root);
    lineStack.push(makeLines(communicator, atime, lines, root));

    size_t progress = 0;
    while (!nodeStack.empty()) {
        progress++; // might need to increase by 2 because it was one for each left/right in the
                    // previous iteration

        if (communicator) {
            if (communicator->IsCancelled()) {
                throw Communicator::CancelledException();
            }
            if (qtimer(atime, 500)) {
                communicator->CommPostMessage(Communicator::CURRENT_RECORD, progress);
            }
        }
        BSPNode *currNode = nodeStack.top();
        nodeStack.pop();
        LineVecPair currLines = lineStack.top();
        lineStack.pop();

        if (!currLines.first.empty()) {
            currNode->left = std::unique_ptr<BSPNode>(new BSPNode(currNode));
            nodeStack.push(currNode->left.get());
            lineStack.push(makeLines(communicator, atime, currLines.first, currNode->left.get()));
        }
        if (!currLines.second.empty()) {
            currNode->right = std::unique_ptr<BSPNode>(new BSPNode(currNode));
            nodeStack.push(currNode->right.get());
            lineStack.push(makeLines(communicator, atime, currLines.second, currNode->right.get()));
        }
    }
}

/* Finds the midpoint from all the lines given and returns the index of the line
 * closest to it.
 */

int BSPTree::pickMidpointLine(const std::vector<Line4f> &lines, BSPNode *par) {
    int chosen = -1;
    Point2f midpoint;
    for (size_t i = 0; i < lines.size(); i++) {
        midpoint += lines[i].start() + lines[i].end();
    }
    midpoint /= 2.0 * static_cast<double>(lines.size());
    bool ver = true;
    if (par && par->getLine().height() > par->getLine().width()) {
        ver = false;
    }
    double chosendist = -1.0;
    for (size_t i = 0; i < lines.size(); i++) {
        const Line4f &line = lines[i];
        if (ver) {
            if (line.height() > line.width() &&
                (chosen == -1 || line.midpoint().dist(midpoint) < chosendist)) {
                chosen = static_cast<int>(i);
                chosendist = line.midpoint().dist(midpoint);
            }
        } else {
            if (line.width() > line.height() &&
                (chosen == -1 || line.midpoint().dist(midpoint) < chosendist)) {
                chosen = static_cast<int>(i);
                chosendist = line.midpoint().dist(midpoint);
            }
        }
    }
    // argh... and again... there weren't any hoz / ver:
    if (chosen == -1) {
        for (size_t i = 0; i < lines.size(); i++) {
            if (chosen == -1 || lines[i].midpoint().dist(midpoint) < chosendist) {
                chosen = static_cast<int>(i);
                chosendist = lines[i].midpoint().dist(midpoint);
            }
        }
    }
    return chosen;
}

/* Breaks a set of lines in two (left-right). First chooses a line closest to the midpoint
 * of the set ("chosen") and then classifies lines left or right depending on whether they
 * lie clockwise or anti-clockwise of the chosen one (with chosen start as centre, angles
 * from the chosen end up to 180 are clockwise, down to -180 anti-clockwise). Lines that cross
 * from one side of the chosen to the other are split in two and each part goes to the relevant set.
 */

std::pair<std::vector<Line4f>, std::vector<Line4f>>
BSPTree::makeLines(Communicator *, time_t, const std::vector<Line4f> &lines, BSPNode *base) {
    std::vector<Line4f> leftlines;
    std::vector<Line4f> rightlines;

    // for optimization of the tree (this reduced a six-minute gen time to a 38 second gen time)
    int chosen = -1;
    if (lines.size() > 3) {
        chosen = BSPTree::pickMidpointLine(lines, base->parent);
        if (chosen < 0) {
            throw new depthmapX::RuntimeException("Error, failed to pick midpoint line");
        }
    } else {
        // TODO: This was originally `chosen = pafrand() % lines.size();`, but was making the
        // isovists non-reproducible. Just pick the first line for the moment. Figure out later
        chosen = 0;
    }

    const Line4f &chosenLine = lines[static_cast<unsigned int>(chosen)];
    base->setLine(chosenLine);

    Point2f v0 = chosenLine.end() - chosenLine.start();
    v0.normalise();

    for (size_t i = 0; i < lines.size(); i++) {
        if (i == static_cast<unsigned int>(chosen)) {
            continue;
        }
        const Line4f &testline = lines[i];
        Point2f v1 = testline.start() - chosenLine.start();
        v1.normalise();
        Point2f v2 = testline.end() - chosenLine.start();
        v2.normalise();
        // should use approxeq here:
        double a = testline.start() == chosenLine.start() ? 0 : v0.det(v1);
        double b = testline.end() == chosenLine.start() ? 0 : v0.det(v2);
        // note sure what to do if a == 0 and b == 0 (i.e., it's parallel... this test at least
        // ensures on the line is one or the other side)
        if (a >= 0 && b >= 0) {
            leftlines.push_back(testline);
        } else if (a <= 0 && b <= 0) {
            rightlines.push_back(testline);
        } else {
            Point2f p = chosenLine.intersection_point(testline);
            Line4f x = Line4f(testline.start(), p);
            Line4f y = Line4f(p, testline.end());
            if (a >= 0) {
                if (x.length() > 0.0) // should use a tolerance here too
                    leftlines.push_back(x);
                if (y.length() > 0.0) // should use a tolerance here too
                    rightlines.push_back(y);
            } else {
                if (x.length() > 0.0) // should use a tolerance here too
                    rightlines.push_back(x);
                if (y.length() > 0.0) // should use a tolerance here too
                    leftlines.push_back(y);
            }
        }
    }

    return std::make_pair(leftlines, rightlines);
}
