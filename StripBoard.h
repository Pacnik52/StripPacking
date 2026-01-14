// StripBoard.h
#pragma once
#include <vector>
#include "BinPackData.h"

struct Rect {
    int x, y, w, h;
};

struct Point {
    int x, y;
};

class StripBoard {
private:
    int stripWidth;
    int currentHeight;
    std::vector<Rect> placedItems;
    // Reprezentacja Skyline: wektor wysokości dla każdej współrzędnej X (prosta implementacja)
    std::vector<int> skyline;

public:
    StripBoard(int width);

    void place(const Rect& rect);

    // Generuje poprawne Corner Points (zgodnie z algorytmem Martello/Skyline)
    std::vector<Point> getCornerPoints() const;

    // Sprawdza czy prostokąt mieści się w danym miejscu (brak kolizji, w granicach)
    bool fits(int x, int y, int w, int h) const;

    // Pobiera profil wysokości w 8 punktach (do cech sieci)
    std::vector<double> getSkylineProfile(int numPoints, int currentMaxH) const;

    // Oblicza "zmarnowaną przestrzeń" pod potencjalnym nowym elementem
    double calculateWastedSpace(int x, int y, int w, int h) const;

    int getHeight() const { return currentHeight; }
    int getWidth() const { return stripWidth; }
    const std::vector<Rect>& getPlacedItems() const { return placedItems; }
};