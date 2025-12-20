#pragma once
#include "BinpackData.h"
#include "placement_strategies/PlacementStrategy.h"
#include <memory>
#include <vector>

namespace binpack {

class StripPacker {
private:
    BinpackData& data;
    std::unique_ptr<PlacementStrategy> strategy;

public:
    StripPacker(BinpackData& data, std::unique_ptr<PlacementStrategy> strategy);
    void pack(const std::vector<BinpackData::BoxType>& boxes);
};

}
