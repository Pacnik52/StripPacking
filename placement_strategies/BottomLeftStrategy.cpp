#include "BottomLeftStrategy.h"
#include <vector>
#include <algorithm>
#include <iostream>

namespace binpack {

// Comparison function for sorting placement options
bool comparePlacements(const EvaListElementType& a, const EvaListElementType& b) {
    if (a.first.P1.Y != b.first.P1.Y) {
        return a.first.P1.Y < b.first.P1.Y;
    }
    return a.first.P1.X < b.first.P1.X;
}

void BottomLeftStrategy::place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) {
    // 1. Initialize CornerPoints for the strip
    CornerPoints cp(data.PSizeX, data.PSizeY);

    // 2. Clear previous solution
    data.Solution.BPV.clear();
    data.Solution.BPV.reserve(boxes.size());

    // 3. Iterate through each box to place it
    for (const auto& box : boxes) {
        EvaListType evaList;

        // 4a. Evaluate both non-rotated and rotated placements
        cp.Evaluate(&box, false, evaList);
        cp.Evaluate(&box, true, evaList);

        // 4b. If no placement is possible, continue to the next box
        if (evaList.empty()) {
            std::cout << "Warning: Could not place box of size " << box.SizeX << "x" << box.SizeY << std::endl;
            continue;
        }

        // 4c. Sort placements to find the best (bottom-leftmost)
        std::sort(evaList.begin(), evaList.end(), comparePlacements);
        const auto& bestPlacement = evaList.front();

        // 4d. Place the box at the best found position
        cp.Insert(&box, bestPlacement);

        // 4e. Record the placement in the solution
        // We need to find the original index of the box type
        int boxTypeIdx = -1;
        for(size_t i = 0; i < data.BoxTypes.size(); ++i) {
            if (data.BoxTypes[i].SizeX == box.SizeX && data.BoxTypes[i].SizeY == box.SizeY) {
                boxTypeIdx = static_cast<int>(i);
                break;
            }
        }
        if (boxTypeIdx != -1) {
            data.Solution.BPV.emplace_back(boxTypeIdx, BinpackData::Pos(bestPlacement.first.P1.X, bestPlacement.first.P1.Y, bestPlacement.first.Rotated, 0));
        }
    }

    // 5. Calculate the objective (for strip packing, this is the max X coordinate used)
    double x_max = 0;
    for (const auto& placedBox : data.Solution.BPV) {
        const auto& pos = placedBox.second;
        const auto& boxType = data.BoxTypes[placedBox.first];
        int width = pos.Rotated ? boxType.SizeY : boxType.SizeX;
        if (pos.X + width > x_max) {
            x_max = pos.X + width;
        }
    }
    data.Solution.setObj(x_max);

    std::cout << "BottomLeftStrategy::place() finished. Placed " << data.Solution.BPV.size() << " boxes." << std::endl;
}

}
