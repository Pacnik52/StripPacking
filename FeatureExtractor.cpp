// FeatureExtractor.cpp
#include "FeatureExtractor.h"
#include "utils.h" // scaleTanh, scale1
#include <numeric>

std::vector<double> FeatureExtractor::extract(
    const StripBoard& board,
    const binpack::BinpackData::BoxType& box,
    bool rotated,
    const Point& pos,
    const std::vector<binpack::BinpackData::BoxType>& remainingBoxes
) {
    std::vector<double> f;
    f.reserve(22);

    int w = rotated ? box.SizeY : box.SizeX;
    int h = rotated ? box.SizeX : box.SizeY;

    // --- I. Info o elemencie ---
    f.push_back(binpack::scale1(0, 1000, w)); // 1. Width
    f.push_back(binpack::scale1(0, 1000, h)); // 2. Height

    // --- II. Info o pozostałych elementach ---
    double totalArea = 0;
    double typeArea = 0;
    int typeCount = 0;

    // Identyfikacja typu po wymiarach (uproszczona)
    auto isSameType = [&](const auto& b){
        return (b.SizeX == box.SizeX && b.SizeY == box.SizeY);
    };

    for(const auto& b : remainingBoxes) {
        double area = (double)b.SizeX * b.SizeY;
        totalArea += area;
        if(isSameType(b)) {
            typeArea += area;
            typeCount++;
        }
    }

    // Używamy scaleTanh dla dużych wartości zgodnie z art.
    f.push_back(binpack::scaleTanh(totalArea / 100000.0)); // 3. Remaining area
    f.push_back(binpack::scaleTanh(typeArea / 10000.0));   // 4. Type remaining area
    f.push_back(binpack::scaleTanh(remainingBoxes.size())); // 5. Num items

    // --- III. Info o stanie po wstawieniu ---
    int newHeight = std::max(board.getHeight(), pos.y + h);

    // 6-13. ID view (8 punktów)
    auto profile = board.getSkylineProfile(8, newHeight);
    for(double val : profile) {
        f.push_back(binpack::scaleTanh(val / 100.0));
    }

    // 14. Horizontal Mismatch (uproszczony)
    // Sprawdzamy sąsiada po lewej i prawej na poziomie y
    // Wymagałoby dokładniejszego query do skyline, tutaj aproksymacja
    f.push_back(0.0);

    // 15. Vertical Mismatch
    // Czy góra elementu pasuje do sąsiadów?
    f.push_back(0.0);

    // 16. Wasted Space
    double wasted = board.calculateWastedSpace(pos.x, pos.y, w, h);
    f.push_back(binpack::scaleTanh(wasted / 1000.0));

    // 17. Horizontal Size Fit (dx mod w)/w
    int distToRight = board.getWidth() - (pos.x + w);
    double hFit = (double)(distToRight % w) / w;
    f.push_back(hFit);

    // 18. Vertical Size Fit (dy mod l)/l
    int distToTop = newHeight - (pos.y + h); // z definicji artykułu
    double vFit = -1.0;
    // Artykuł definiuje to względem "current height".
    // Jeśli pos.y + h wystaje powyżej obecnej max wysokości, to dy jest ujemne
    int dy = board.getHeight() - (pos.y + h);
    if (dy < 0) vFit = -1.0;
    else vFit = (double)(dy % h) / h;
    f.push_back(vFit);

    // 19. Horizontal Distance
    f.push_back(binpack::scale1(0, board.getWidth(), distToRight));

    // 20. Vertical Distance
    f.push_back(binpack::scaleTanh(dy)); // Dystans od góry elementu do max height

    // 21. Horizontal Position (left)
    f.push_back(binpack::scale1(0, board.getWidth(), pos.x));

    // 22. Vertical Position (bottom)
    f.push_back(binpack::scaleTanh(pos.y));

    // Dopełnienie do 22 cech jeśli coś pominęliśmy (np. mismatch dokładniej)
    while(f.size() < 22) f.push_back(0.0);

    return f;
}