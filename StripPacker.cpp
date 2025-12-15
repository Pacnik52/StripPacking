#include "StripPacker.h"

namespace binpack {

StripPacker::StripPacker(BinpackData& data, std::unique_ptr<PlacementStrategy> strategy)
    : data(data), strategy(std::move(strategy)) {
}

void StripPacker::pack(const std::vector<BinpackData::BoxType>& boxes) {
    // Delegate the actual packing to the strategy object.
    if (strategy) {
        strategy->place(boxes, data);
    }
}

}
