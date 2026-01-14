#pragma once
#include <vector>
#include "BinPackData.h"
#include "StripBoard.h"

class FeatureExtractor {
public:
    static std::vector<double> extract(
        const StripBoard& board,
        const binpack::BinpackData::BoxType& box,
        bool rotated,
        const binpack::BinpackData::Pos& pos,
        const std::vector<binpack::BinpackData::BoxType>& remainingBoxes
    );
};