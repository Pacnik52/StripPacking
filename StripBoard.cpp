#include "StripBoard.h"
#include <algorithm>
#include <iostream>

StripBoard::StripBoard(int width) : stripWidth(width), currentHeight(0) {
    SkylineNode initialNode;
    initialNode.x = 0;
    initialNode.y = 0;
    initialNode.width = width;
    skyline.push_back(initialNode);
}

int StripBoard::getSkylineHeightAt(int x) const {
    for (const auto& node : skyline) {
        if (x >= node.x && x < node.x + node.width) {
            return node.y;
        }
    }
    return 0;
}

bool StripBoard::fits(int x, int y, int w, int h) const {
    if (x + w > stripWidth) return false;
    for (const auto& node : skyline) {
        if (node.x < x + w && node.x + node.width > x) {
            if (node.y > y) return false; // collision
        }
    }
    return true;
}

std::vector<binpack::BinpackData::Pos> StripBoard::getCornerPoints() const {
    std::vector<binpack::BinpackData::Pos> points;
    for (const auto& node : skyline) {
        binpack::BinpackData::Pos p;
        p.X = node.x;
        p.Y = node.y;
        points.push_back(p);
    }
    return points;
}

void StripBoard::place(const Rect& rect) {
    placedItems.push_back(rect);
    int newTop = rect.y + rect.h;
    if (newTop > currentHeight) currentHeight = newTop;

    std::vector<SkylineNode> newSkyline;

    for (const auto& node : skyline) {
        if (node.x + node.width <= rect.x) {
            newSkyline.push_back(node);
        }
        else if (node.x >= rect.x + rect.w) {
            newSkyline.push_back(node);
        }
        else {
            if (node.x < rect.x) {
                newSkyline.push_back({node.x, node.y, rect.x - node.x});
            }

            // Środek (przykryty przez nowy blok) jest teraz wyższy
            // UWAGA: Tutaj nie dodajemy segmentu od razu, bo nowy blok może
            // przykrywać wiele starych segmentów. Dodamy jeden duży segment na końcu pętli
            // albo obsłużymy to sprytniej.

            // Część wystająca z prawej?
            if (node.x + node.width > rect.x + rect.w) {
                 // Ten fragment zostanie dodany później, po wstawieniu segmentu bloku
                 // Ale w tej pętli prościej jest po prostu dodać segment bloku w odpowiednim momencie.
            }
        }
    }
    std::vector<SkylineNode> nextSkyline;

    for (const auto& node : skyline) {
        if (node.x + node.width <= rect.x || node.x >= rect.x + rect.w) {
            nextSkyline.push_back(node);
        }
        else {
            // Fits left
            if (node.x < rect.x) {
                nextSkyline.push_back({node.x, node.y, rect.x - node.x});
            }
            // Fits right?
            if (node.x + node.width > rect.x + rect.w) {
                nextSkyline.push_back({rect.x + rect.w, node.y, (node.x + node.width) - (rect.x + rect.w)});
            }
        }
    }
    // adding new node to the skyline in the correct position
    SkylineNode newNode = {rect.x, rect.y + rect.h, rect.w};
    auto it = nextSkyline.begin();
    while (it != nextSkyline.end() && it->x < newNode.x) {
        it++;
    }
    nextSkyline.insert(it, newNode);

    // merge same height nodes
    std::vector<SkylineNode> merged;
    if (!nextSkyline.empty()) {
        merged.push_back(nextSkyline[0]);
        for (size_t i = 1; i < nextSkyline.size(); ++i) {
            SkylineNode& prev = merged.back();
            SkylineNode& curr = nextSkyline[i];

            if (prev.y == curr.y && prev.x + prev.width == curr.x) {
                prev.width += curr.width; // Scal
            } else {
                merged.push_back(curr);
            }
        }
    }
    skyline = merged;
}

double StripBoard::calculateWastedSpace(int x, int y, int w, int h) const {
    // area under the new box and above the skyline
    double wasted = 0;

    for (const auto& node : skyline) {
        int intersectX1 = std::max(node.x, x);
        int intersectX2 = std::min(node.x + node.width, x + w);

        if (intersectX2 > intersectX1) {
            int width = intersectX2 - intersectX1;
            int heightDiff = y - node.y;
            wasted += (double)width * heightDiff;
        }
    }
    return wasted;
}

std::vector<double> StripBoard::getSkylineProfile(int numPoints, int currentMaxH) const {
    std::vector<double> profile;
    double step = (double)stripWidth / (numPoints + 1);

    for(int i=1; i<=numPoints; ++i) {
        int sampleX = (int)(step * i);
        if (sampleX >= stripWidth) sampleX = stripWidth - 1;

        int skyH = getSkylineHeightAt(sampleX);
        profile.push_back((double)(currentMaxH - skyH));
    }
    return profile;
}