#pragma once
#include <vector>
#include "BinPackData.h"

struct Rect {
    int x, y, w, h;
};

struct SkylineNode {
    int x;
    int y;
    int width;
};

class StripBoard {
private:
    int stripWidth;
    int currentHeight;
    std::vector<Rect> placedItems;
    std::vector<SkylineNode> skyline;

public:
    StripBoard(int width);

    void place(const Rect& rect);

    std::vector<binpack::BinpackData::Pos> getCornerPoints() const;

    bool fits(int x, int y, int w, int h) const;

    double calculateWastedSpace(int x, int y, int w, int h) const;

    std::vector<double> getSkylineProfile(int numPoints, int currentMaxH) const;

    int getHeight() const { return currentHeight; }

    int getWidth() const { return stripWidth; }

    int getSkylineHeightAt(int x) const;
};