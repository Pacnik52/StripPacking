#include "MaxRectsStrategy.h"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace binpack {

MaxRectsStrategy::MaxRectsStrategy(FreeRectChoiceHeuristic heuristic) 
    : method(heuristic) {}

void MaxRectsStrategy::init(int width, int height) {
    freeRects.clear();
    freeRects.push_back({0, 0, width, height});
}

void MaxRectsStrategy::place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) {
    // For Strip Packing, we assume fixed Width and infinite Height.
    // However, MaxRects needs a finite space to calculate splits.
    // We use data.PSizeY (assuming it's set large enough, e.g., 10000 or the max bound)
    init(data.PSizeX, data.PSizeY);

    data.Solution.BPV.clear();
    data.Solution.BPV.reserve(boxes.size());

    // Sorting inputs often helps MaxRects significantly (Descent by Area/Height)
    // You can uncomment this to enable sorting:
    /*
    std::vector<BinpackData::BoxType> sortedBoxes = boxes;
    std::sort(sortedBoxes.begin(), sortedBoxes.end(), [](const auto& a, const auto& b) {
         return (a.SizeX * a.SizeY) > (b.SizeX * b.SizeY);
    });
    const auto& boxesToProcess = sortedBoxes;
    */
    const auto& boxesToProcess = boxes; // Use unsorted for now

    for (const auto& box : boxesToProcess) {
        int bestScore1 = std::numeric_limits<int>::max();
        int bestScore2 = std::numeric_limits<int>::max();
        Rect bestNode = {0, 0, 0, 0};
        bool bestRotated = false;
        bool found = false;

        // 1. Try placing without rotation
        int score1, score2;
        Rect newNode = scoreRect(box.SizeX, box.SizeY, method, score1, score2);

        if (newNode.height > 0) { // Valid placement found
            bestScore1 = score1;
            bestScore2 = score2;
            bestNode = newNode;
            bestRotated = false;
            found = true;
        }

        // 2. Try placing with rotation
        Rect newNodeRot = scoreRect(box.SizeY, box.SizeX, method, score1, score2);
        if (newNodeRot.height > 0) {
            if (!found || score1 < bestScore1 || (score1 == bestScore1 && score2 < bestScore2)) {
                bestScore1 = score1;
                bestScore2 = score2;
                bestNode = newNodeRot;
                bestRotated = true;
                found = true;
            }
        }

        // 3. Commit the placement
        if (found) {
            placeRect(bestNode);
            
            // Find the original box index for the solution
            // (If boxes are sorted, this mapping might need care, but for now we assume direct mapping logic or ID usage)
            int boxTypeIdx = -1;
            // Simple lookup matching dimensions (assuming unique types or generic handling)
            for(size_t i = 0; i < data.BoxTypes.size(); ++i) {
                if (data.BoxTypes[i].idx == box.idx) {
                     boxTypeIdx = static_cast<int>(i);
                     break;
                }
            }
            // Fallback for demonstration
            if (boxTypeIdx == -1) boxTypeIdx = box.idx;

            data.Solution.BPV.emplace_back(boxTypeIdx, BinpackData::Pos(bestNode.x, bestNode.y, bestRotated, 0));
        }
    }

    // 4. Calculate Objective (Max Height Used)
    double y_max = 0;
    for (const auto& placed : data.Solution.BPV) {
        const auto& pos = placed.second;
        const auto& box = data.BoxTypes[placed.first];
        int h = pos.Rotated ? box.SizeX : box.SizeY;
        if (pos.Y + h > y_max) y_max = pos.Y + h;
    }
    data.Solution.setObj(y_max);
}

// --- MaxRects Core Logic ---

MaxRectsStrategy::Rect MaxRectsStrategy::scoreRect(int width, int height, FreeRectChoiceHeuristic method, int& score1, int& score2) const {
    Rect newNode{0, 0, 0, 0};
    score1 = std::numeric_limits<int>::max();
    score2 = std::numeric_limits<int>::max();

    for (const auto& free : freeRects) {
        if (free.width >= width && free.height >= height) {
            int currentScore1 = 0;
            int currentScore2 = 0;
            
            if (method == FreeRectChoiceHeuristic::RectBestShortSideFit) {
                int leftoverHoriz = std::abs(free.width - width);
                int leftoverVert = std::abs(free.height - height);
                currentScore1 = std::min(leftoverHoriz, leftoverVert);
                currentScore2 = std::max(leftoverHoriz, leftoverVert);
            }
            else { 
                // Default / Bottom Left Rule
                currentScore1 = free.y + height;
                currentScore2 = free.x;
            }

            if (currentScore1 < score1 || (currentScore1 == score1 && currentScore2 < score2)) {
                score1 = currentScore1;
                score2 = currentScore2;
                newNode.x = free.x;
                newNode.y = free.y;
                newNode.width = width;
                newNode.height = height;
            }
        }
    }
    return newNode;
}

void MaxRectsStrategy::placeRect(const Rect& placedNode) {
    size_t numRectsToProcess = freeRects.size();
    for (size_t i = 0; i < numRectsToProcess; ++i) {
        if (splitFreeNode(freeRects[i], placedNode)) {
            freeRects.erase(freeRects.begin() + i);
            --i;
            --numRectsToProcess;
        }
    }
    pruneFreeList();
}

bool MaxRectsStrategy::splitFreeNode(Rect freeNode, const Rect& usedNode) {
    // 1. Check Intersection
    if (usedNode.x >= freeNode.x + freeNode.width || usedNode.x + usedNode.width <= freeNode.x ||
        usedNode.y >= freeNode.y + freeNode.height || usedNode.y + usedNode.height <= freeNode.y)
        return false;

    // 2. Split into up to 4 new rectangles
    if (usedNode.x < freeNode.x + freeNode.width && usedNode.x + usedNode.width > freeNode.x) {
        // New node at the top side of the used node
        if (usedNode.y > freeNode.y && usedNode.y < freeNode.y + freeNode.height) {
            Rect newNode = freeNode;
            newNode.height = usedNode.y - newNode.y;
            freeRects.push_back(newNode);
        }
        // New node at the bottom side of the used node
        if (usedNode.y + usedNode.height < freeNode.y + freeNode.height) {
            Rect newNode = freeNode;
            newNode.y = usedNode.y + usedNode.height;
            newNode.height = freeNode.y + freeNode.height - (usedNode.y + usedNode.height);
            freeRects.push_back(newNode);
        }
    }

    if (usedNode.y < freeNode.y + freeNode.height && usedNode.y + usedNode.height > freeNode.y) {
        // New node at the left side of the used node
        if (usedNode.x > freeNode.x && usedNode.x < freeNode.x + freeNode.width) {
            Rect newNode = freeNode;
            newNode.width = usedNode.x - newNode.x;
            freeRects.push_back(newNode);
        }
        // New node at the right side of the used node
        if (usedNode.x + usedNode.width < freeNode.x + freeNode.width) {
            Rect newNode = freeNode;
            newNode.x = usedNode.x + usedNode.width;
            newNode.width = freeNode.x + freeNode.width - (usedNode.x + usedNode.width);
            freeRects.push_back(newNode);
        }
    }
    return true;
}

void MaxRectsStrategy::pruneFreeList() {
    // Remove rectangles that are entirely contained within other free rectangles
    for (size_t i = 0; i < freeRects.size(); ++i) {
        for (size_t j = i + 1; j < freeRects.size(); ++j) {
            if (freeRects[j].x >= freeRects[i].x && freeRects[j].y >= freeRects[i].y &&
                freeRects[j].x + freeRects[j].width <= freeRects[i].x + freeRects[i].width &&
                freeRects[j].y + freeRects[j].height <= freeRects[i].y + freeRects[i].height) {
                freeRects.erase(freeRects.begin() + j);
                --j;
            }
            else if (freeRects[i].x >= freeRects[j].x && freeRects[i].y >= freeRects[j].y &&
                     freeRects[i].x + freeRects[i].width <= freeRects[j].x + freeRects[j].width &&
                     freeRects[i].y + freeRects[i].height <= freeRects[j].y + freeRects[j].height) {
                freeRects.erase(freeRects.begin() + i);
                --i;
                break;
            }
        }
    }
}

}