#pragma once
#include "BinpackData.h"
#include <vector>

namespace binpack {

class PlacementStrategy {
public:
    virtual ~PlacementStrategy() = default;

    // The main method that every strategy must implement.
    // It takes a list of boxes to pack and a reference to the BinpackData
    // which holds the bin information and where the solution will be stored.
    virtual void place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) = 0;
};

}
