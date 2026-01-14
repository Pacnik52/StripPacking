#include "FeatureExtractor.h"
#include "utils.h"
#include <algorithm>

std::vector<double> FeatureExtractor::extract(
    const StripBoard& board,
    const binpack::BinpackData::BoxType& box,
    bool rotated,
    const binpack::BinpackData::Pos& pos,
    const std::vector<binpack::BinpackData::BoxType>& remainingBoxes
) {
    std::vector<double> f;
    f.reserve(22);

    int w = rotated ? box.SizeY : box.SizeX;
    int h = rotated ? box.SizeX : box.SizeY;

    // width
    f.push_back(binpack::scale1(0, 1000, w));
    // height
    f.push_back(binpack::scale1(0, 1000, h));
    // total area of the box
    double totalArea = 0;
    // area of boxes of the same type
    double typeArea = 0;
    auto isSameType = [&](const auto& b){
        return (b.SizeX == box.SizeX && b.SizeY == box.SizeY);
    };
    for(const auto& b : remainingBoxes) {
        double area = (double)b.SizeX * b.SizeY;
        totalArea += area;
        if(isSameType(b)) typeArea += area;
    }
    f.push_back(binpack::scaleTanh(totalArea / 1000000.0));
    f.push_back(binpack::scaleTanh(typeArea / 100000.0));
    // total area of the remaining boxes
    f.push_back(binpack::scaleTanh(remainingBoxes.size() / 100.0));

    int newTop = pos.Y + h;
    int currentH = std::max(board.getHeight(), newTop);

    // skyline profile
    auto profile = board.getSkylineProfile(8, newTop);
    for(double val : profile) {
        f.push_back(binpack::scaleTanh(val / 100.0));
    }

    // horizontal mismatch
    int leftH = (pos.X > 0) ? board.getSkylineHeightAt(pos.X - 1) : 99999; // ściana
    int rightH = (pos.X + w < board.getWidth()) ? board.getSkylineHeightAt(pos.X + w) : 99999; // ściana
    double mismatch = 0;
    if (leftH < pos.Y + h) mismatch += (pos.Y + h - leftH);
    if (rightH < pos.Y + h) mismatch += (pos.Y + h - rightH);
    f.push_back(binpack::scaleTanh(mismatch / 100.0));

    // vertical mismatch
    double vMismatch = 0;
    if (pos.Y != leftH && leftH != 99999) vMismatch += std::abs(pos.Y - leftH);
    if (pos.Y != rightH && rightH != 99999) vMismatch += std::abs(pos.Y - rightH);
    f.push_back(binpack::scaleTanh(vMismatch / 100.0));

    // wasted space
    double wasted = board.calculateWastedSpace(pos.X, pos.Y, w, h);
    f.push_back(binpack::scaleTanh(wasted / 5000.0));

    // hirozontal size fit
    int distToRight = board.getWidth() - (pos.X + w);
    double hFit = (w > 0) ? (double)(distToRight % w) / w : 0;
    f.push_back(hFit);

    // vertical Size Fit
    int distToTop = currentH - (pos.Y + h);
    double vFit = (h > 0) ? (double)(abs(distToTop) % h) / h : 0;
    f.push_back(vFit);

    // horizontal Distance
    f.push_back(binpack::scale1(0, board.getWidth(), distToRight));

    // vertical Distance
    f.push_back(binpack::scaleTanh((double)distToTop / 100.0));

    // horizontal Pos
    f.push_back(binpack::scale1(0, board.getWidth(), pos.X));

    // vertical Pos
    f.push_back(binpack::scaleTanh(pos.Y / 1000.0));

    // for now!
    while(f.size() < 22) f.push_back(0.0);

    return f;
}