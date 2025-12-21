#pragma once
#include "PlacementStrategy.h"
#include "../BinpackData.h"
#include <vector>
#include <limits>

namespace binpack {

    class MaxRectsStrategy : public PlacementStrategy {
    public:
        // Heuristics for choosing the best free space
        enum class FreeRectChoiceHeuristic {
            RectBestShortSideFit, // BSSF: Generally the best performer
            RectBestLongSideFit,  // BLSF
            RectBestAreaFit,      // BAF
            RectBottomLeftRule,   // BL
            RectContactPointRule  // CP
        };

        explicit MaxRectsStrategy(FreeRectChoiceHeuristic heuristic = FreeRectChoiceHeuristic::RectBestShortSideFit);

        void place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) override;

    private:
        // Internal structure to track empty space (Different from CornerPoints!)
        struct Rect {
            int x, y, width, height;
        };

        std::vector<Rect> freeRects;
        FreeRectChoiceHeuristic method;

        // --- Core MaxRects Logic ---
        void init(int width, int height);

        // Finds the best free rectangle for a specific box
        Rect scoreRect(int width, int height, FreeRectChoiceHeuristic method, int& score1, int& score2) const;

        // Places the box and splits the free rectangles
        void placeRect(const Rect& node);

        // Splits a free rectangle if it overlaps with the placed box
        bool splitFreeNode(Rect freeNode, const Rect& usedNode);

        // Removes free rectangles that are contained within others
        void pruneFreeList();
    };

}