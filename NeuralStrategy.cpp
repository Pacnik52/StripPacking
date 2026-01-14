#include "NeuralStrategy.h"
#include "StripBoard.h"
#include "FeatureExtractor.h"
#include <limits>

NeuralStrategy::NeuralStrategy(NeuralNetwork& network) : net(network) {}

double NeuralStrategy::solve(binpack::BinpackData& data) {
    StripBoard board(data.PSizeX);

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

        binpack::BinpackData::Pos bestPos;
        bestPos.X = -1;
        bestPos.Y = -1;

        bool foundAnyMove = false;

        auto corners = board.getCornerPoints();

        // for every box in remaining
        for(size_t i=0; i<remaining.size(); ++i) {
            // if that type of box was already checked
            if (i > 0 && remaining[i] == remaining[i-1]) continue;

            const auto& box = remaining[i];

            // for every rotation
            for(int r=0; r<2; ++r) {
                bool rot = (r == 1);
                int w = rot ? box.SizeY : box.SizeX;
                int h = rot ? box.SizeX : box.SizeY;

                // for every corner point
                for(const auto& pt : corners) {
                    if (board.fits(pt.X, pt.Y, w, h)) {
                        foundAnyMove = true;

                        auto features = FeatureExtractor::extract(board, box, rot, pt, remaining);

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

            // ZMIANA: board.place oczekuje Rect {x, y, w, h}
            // Używamy .X i .Y ze znalezionego bestPos
            board.place({bestPos.X, bestPos.Y, w, h});

            // Zapisz wynik
            binpack::BinpackData::Pos pos;
            pos.X = bestPos.X;
            pos.Y = bestPos.Y;
            pos.Rotated = bestRotated;

            placedPositions.push_back({box.idx, pos});

            // Usuń z listy
            remaining.erase(remaining.begin() + bestIdx);
        } else {
            // Fallback strategy (gdyby sieć/cornery nie znalazły miejsca, co jest rzadkie w StripPacking)
            if (!foundAnyMove && !remaining.empty()) {
                const auto& box = remaining[0];
                int y = board.getHeight();

                // Kładziemy na samej górze (bezpieczne miejsce)
                board.place({0, y, box.SizeX, box.SizeY});

                binpack::BinpackData::Pos pos;
                pos.X = 0;
                pos.Y = y;
                pos.Rotated = false;

                placedPositions.push_back({box.idx, pos});
                remaining.erase(remaining.begin());
            } else {
                break;
            }
        }
    }

    // Finalizacja
    data.Solution.BPV = placedPositions;
    data.Solution.obj = board.getHeight();

    // Oblicz Fill Factor
    double stripArea = (double)data.PSizeX * board.getHeight();
    if (stripArea == 0) return 0.0;

    return totalItemsArea / stripArea;
}