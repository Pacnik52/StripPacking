#pragma once
#include "PlacementStrategy.h"
#include "../CornerPoints.h"

namespace binpack {

class BottomLeftStrategy : public PlacementStrategy {
public:
    void place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) override;
};

}
