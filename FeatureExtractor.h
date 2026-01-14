// FeatureExtractor.h
#pragma once
#include <vector>
#include "BinPackData.h"
#include "StripBoard.h"

class FeatureExtractor {
public:
    // Pobiera cechy dla konkretnego ruchu (typ pudełka, obrót, pozycja)
    static std::vector<double> extract(
        const StripBoard& board,
        const binpack::BinpackData::BoxType& box,
        bool rotated,
        const Point& pos,
        const std::vector<binpack::BinpackData::BoxType>& remainingBoxes
    );
};