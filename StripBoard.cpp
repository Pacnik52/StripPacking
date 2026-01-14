// StripBoard.cpp
#include "StripBoard.h"
#include <algorithm>
#include <iostream>

StripBoard::StripBoard(int width) : stripWidth(width), currentHeight(0) {
    skyline.resize(width, 0);
}

void StripBoard::place(const Rect& rect) {
    placedItems.push_back(rect);
    currentHeight = std::max(currentHeight, rect.y + rect.h);

    // Aktualizacja skyline
    for(int i = rect.x; i < rect.x + rect.w; ++i) {
        if (i < stripWidth) {
            skyline[i] = std::max(skyline[i], rect.y + rect.h);
        }
    }
}

bool StripBoard::fits(int x, int y, int w, int h) const {
    if (x + w > stripWidth) return false;
    // Sprawdzenie kolizji z istniejącym skyline (optymalizacja)
    for (int i = x; i < x + w; ++i) {
        if (skyline[i] > y) return false;
    }
    return true;
}

std::vector<Point> StripBoard::getCornerPoints() const {
    std::vector<Point> points;
    // Zawsze dostępny punkt (0, skyline[0]) - uproszczenie dla startu
    points.push_back({0, skyline[0]});

    // Punkty narożne powstają na załamaniach skyline ("wnękach")
    for (int i = 1; i < stripWidth; ++i) {
        if (skyline[i] < skyline[i-1]) {
            points.push_back({i, skyline[i]});
        }
    }
    // Dodatkowo: punkty wynikające z prawej krawędzi już położonych elementów
    // (W pełnej implementacji Martello algorytm jest bardziej złożony,
    // tutaj wersja uproszczona oparta na skyline, wystarczająca dla metody konstrukcyjnej)
    return points;
}

std::vector<double> StripBoard::getSkylineProfile(int numPoints, int queryH) const {
    std::vector<double> profile;
    double step = (double)stripWidth / (numPoints + 1);
    for(int i=1; i<=numPoints; ++i) {
        int x = (int)(step * i);
        if (x >= stripWidth) x = stripWidth - 1;
        // Cecha: dystans od queryH do skyline
        profile.push_back((double)(queryH - skyline[x]));
    }
    return profile;
}

double StripBoard::calculateWastedSpace(int x, int y, int w, int h) const {
    double wasted = 0;
    // Obszar bezpośrednio pod nowym elementem, który jest pusty
    for (int i = x; i < x + w; ++i) {
        wasted += (y - skyline[i]);
    }
    return wasted;
}