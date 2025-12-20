#pragma once
#include "../BinpackData.h"
#include <vector>

namespace binpack {

class PlacementStrategy {
public:
    virtual ~PlacementStrategy() = default;

    virtual void place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) = 0;
};

}
