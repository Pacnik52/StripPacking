#include "StripPacker.h"

namespace binpack {

StripPacker::StripPacker(BinpackData& data, std::unique_ptr<PlacementStrategy> strategy)
    : data(data), strategy(std::move(strategy)) {
}

void StripPacker::pack(const std::vector<BinpackData::BoxType>& boxes) {
    if (strategy) {
        strategy->place(boxes, data);
    }
}

}
