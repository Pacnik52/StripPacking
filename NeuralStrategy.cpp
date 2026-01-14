// NeuralStrategy.cpp
#include "NeuralStrategy.h"
#include "StripBoard.h"
#include "FeatureExtractor.h"
#include <limits>
#include <iostream>

NeuralStrategy::NeuralStrategy(NeuralNetwork& network) : net(network) {}

double NeuralStrategy::solve(binpack::BinpackData& data) {
    StripBoard board(data.PSizeX);

    // Przygotowanie listy wszystkich pudełek do ułożenia (rozpakowanie BoxToLoad)
    std::vector<binpack::BinpackData::BoxType> remaining;
    for(size_t i=0; i<data.BoxToLoad.size(); ++i) {
        for(int k=0; k<data.BoxToLoad[i]; ++k) {
            remaining.push_back(data.BoxTypes[i]);
        }
    }

    double totalItemsArea = 0;
    for(const auto& b : remaining) totalItemsArea += (b.SizeX * b.SizeY);

    std::vector<std::pair<int, binpack::BinpackData::Pos>> placedPositions;

    while(!remaining.empty()) {
        double bestScore = -std::numeric_limits<double>::infinity();
        int bestIdx = -1;
        bool bestRotated = false;
        Point bestPos = {-1, -1};
        bool foundAnyMove = false;

        // Pobierz możliwe punkty wstawienia
        auto corners = board.getCornerPoints();

        // 1. Iteruj po wszystkich dostępnych typach pudełek (tylko unikalne typy dla wydajności)
        // W artykule: "Only those element types for which there are still unpacked elements"
        for(size_t i=0; i<remaining.size(); ++i) {
            // Mała optymalizacja: jeśli mamy 100 takich samych pudełek, sprawdzamy tylko pierwsze
            if (i > 0 && remaining[i] == remaining[i-1]) continue;

            const auto& box = remaining[i];

            // 2. Iteruj po rotacjach (0 i 90 stopni)
            for(int r=0; r<2; ++r) {
                bool rot = (r == 1);
                int w = rot ? box.SizeY : box.SizeX;
                int h = rot ? box.SizeX : box.SizeY;

                // 3. Iteruj po punktach narożnych
                for(const auto& pt : corners) {
                    if (board.fits(pt.x, pt.y, w, h)) {
                        foundAnyMove = true;

                        // Ekstrakcja cech dla tego ruchu
                        auto features = FeatureExtractor::extract(board, box, rot, pt, remaining);

                        // Ocena siecią
                        double score = net.predict(features);

                        if (score > bestScore) {
                            bestScore = score;
                            bestIdx = i;
                            bestRotated = rot;
                            bestPos = pt;
                        }
                    }
                }
            }
        }

        if (bestIdx != -1) {
            // Wykonaj najlepszy ruch
            const auto& box = remaining[bestIdx];
            int w = bestRotated ? box.SizeY : box.SizeX;
            int h = bestRotated ? box.SizeX : box.SizeY;

            board.place({bestPos.x, bestPos.y, w, h});

            // Zapisz wynik w formacie oczekiwanym przez BinpackData
            binpack::BinpackData::Pos pos;
            pos.X = bestPos.x;
            pos.Y = bestPos.y;
            pos.Rotated = bestRotated;
            // Musimy znaleźć oryginalny indeks typu pudełka
            // (w tym uproszczonym kodzie zakładamy, że idx w BoxType jest poprawny)
            placedPositions.push_back({box.idx, pos});

            // Usuń z listy do zrobienia
            remaining.erase(remaining.begin() + bestIdx);
        } else {
            // Nie znaleziono ruchu (np. brak miejsca w punktach narożnych - rzadkie przy infinite height)
            // W takiej sytuacji można spróbować położyć "gdziekolwiek" na górze,
            // ale Strip Packing zazwyczaj zawsze pozwala położyć coś na górze skyline.
            if (!foundAnyMove && !remaining.empty()) {
                // Fallback: połóż pierwszy element na (0, maxH)
                const auto& box = remaining[0];
                int y = board.getHeight();
                board.place({0, y, box.SizeX, box.SizeY});
                binpack::BinpackData::Pos pos(0, y, false, 0);
                placedPositions.push_back({box.idx, pos});
                remaining.erase(remaining.begin());
            } else {
                break; // Powinno być niemożliwe w Strip Packing
            }
        }
    }

    // Zapisz rozwiązanie do obiektu
    data.Solution.BPV = placedPositions;
    data.Solution.obj = board.getHeight(); // Minimalizujemy wysokość

    // Oblicz Fill Factor
    double stripArea = (double)data.PSizeX * board.getHeight();
    if (stripArea == 0) return 0.0;
    return totalItemsArea / stripArea;
}