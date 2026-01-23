#include <random>
#include <SFML/Graphics.hpp>
#include "BinDrawer.h"
#include "../utils.h"
#include <set>
#include <string>
#include <algorithm>
#include <climits>
#include <iostream>

namespace binpack {
    void BinDrawer::populateColors() {
        Colors.clear();
        for( int r = 0; r < 4; r++ ) {
            for( int g = 0; g < 4; g++ ) {
                for( int b = 0; b < 4; b++ ) {
                    if (r == b && g == b && r == g) continue;
                    Colors.push_back( sf::Color( r*64, g*64, b*64 ) );
                }
            }
        }
        std::shuffle(Colors.begin(), Colors.end(), std::mt19937(1));
    }

void BinDrawer::drawToFile(const BinpackData &IOD, bool flip, string dir, string ext) {
    set<int> Bins;
    for (auto &BP : IOD.Solution.BPV) Bins.insert(BP.second.binIdx);

    for (auto binIdx : Bins) {
        // 1. Obliczanie faktycznych wymiarów zajętej przestrzeni
        long long maxHeuristicX = 0;
        long long maxHeuristicY = 0;

        for(auto &BP : IOD.Solution.BPV) {
            if (BP.second.binIdx != binIdx) continue;
            auto &B = IOD.BoxTypes[BP.first];
            auto Pos = BP.second;
            int itemLen = B.SizeX;
            int itemWid = B.SizeY;
            if (Pos.Rotated) swap(itemLen, itemWid);
            if (Pos.X + itemLen > maxHeuristicX) maxHeuristicX = Pos.X + itemLen;
            if (Pos.Y + itemWid > maxHeuristicY) maxHeuristicY = Pos.Y + itemWid;
        }

        // logicWidth to szerokość paska (stała dla zadania)
        // logicLength to wysokość ułożenia (zmienna dla zadania)
        float logicLength = (float)maxHeuristicX;
        float logicWidth  = (float)maxHeuristicY;
        if (logicLength <= 0) logicLength = 100.0f;
        if (logicWidth <= 0) logicWidth = 100.0f;

        // --- KLUCZOWA ZMIANA: STAŁA SZEROKOŚĆ W PIXELACH ---
        // Definiujemy, ile pikseli szerokości ma mieć sam "pasek" (bez marginesów)
        const float FIXED_STRIP_PX_WIDTH = 800.0f;

        // Skala zależy WYŁĄCZNIE od szerokości paska (logicWidth)
        float SCALE = FIXED_STRIP_PX_WIDTH / logicWidth;

        // Ograniczenie wysokości ze względu na limity techniczne tekstur (np. 16k px)
        if (SCALE * logicLength > 16000.0f) {
            SCALE = 16000.0f / logicLength;
        }

        float MARGIN = 3.0f;
        sf::Color marginColor(0,0,0);

        float CAN_W, CAN_H;
        if (flip) {
            // W wizualizacji pionowej (flip=true):
            // Szerokość obrazka (CAN_W) odpowiada szerokości paska (logicWidth)
            CAN_W = logicWidth * SCALE;  // To zawsze będzie ok. 800px
            CAN_H = logicLength * SCALE; // To będzie zmienne (wysokość wieży)
        } else {
            // W wizualizacji poziomej:
            // Szerokość obrazka (CAN_W) odpowiada długości paska
            CAN_W = logicLength * SCALE;
            CAN_H = logicWidth * SCALE;
        }

        unsigned int imgW = static_cast<unsigned int>(CAN_W + MARGIN * 2);
        unsigned int imgH = static_cast<unsigned int>(CAN_H + MARGIN);

        sf::RenderTexture window;
        if (!window.resize({imgW, imgH})) continue;

        window.clear(marginColor);

        // Białe wnętrze paska
        sf::RectangleShape binBackground({CAN_W, CAN_H});
        binBackground.setFillColor(sf::Color::White);
        binBackground.setPosition({MARGIN, 0.0f});
        window.draw(binBackground);

        // Tekst informacyjny
        sf::Text infoText(font);
        infoText.setString("FF: " + to_string_with_precision(IOD.getObj()*100, 2) + "%  H: " + std::to_string(maxHeuristicX));
        infoText.setCharacterSize(24);
        infoText.setFillColor(sf::Color::Black);
        infoText.setStyle(sf::Text::Bold);

        sf::FloatRect tb = infoText.getLocalBounds();
        infoText.setPosition({ (float)imgW - MARGIN - tb.size.x - 5.0f, 5.0f });

        // Rysowanie pudełek
        sf::Text txtNum(font);
        txtNum.setFillColor(sf::Color::Black);

        for( auto &BP: IOD.Solution.BPV ) {
            if (BP.second.binIdx != binIdx) continue;
            auto &B = IOD.BoxTypes[BP.first];
            auto Pos = BP.second;

            float hLen = B.SizeX * SCALE;
            float hWid = B.SizeY * SCALE;
            if (Pos.Rotated) swap(hLen, hWid);

            float hX = Pos.X * SCALE;
            float hY = Pos.Y * SCALE;

            float drawX, drawY, drawW, drawH;
            if (flip) {
                drawX = MARGIN + hY;
                drawY = CAN_H - hX - hLen;
                drawW = hWid;
                drawH = hLen;
            } else {
                drawX = MARGIN + hX;
                drawY = CAN_H - hY - hWid;
                drawW = hLen;
                drawH = hWid;
            }

            sf::RectangleShape Rect({drawW, drawH});
            Rect.setFillColor(B.idx < Colors.size() ? Colors[B.idx % Colors.size()] : sf::Color(128,128,128));
            Rect.setOutlineColor(sf::Color::Black);
            Rect.setOutlineThickness(1.0f);
            Rect.setPosition({drawX, drawY});
            window.draw(Rect);

            // Numerki (tekst centrowany w pudełku)
            txtNum.setString(std::to_string(B.idx));
            float minSide = std::min(drawW, drawH);
            txtNum.setCharacterSize(static_cast<unsigned int>(std::max(10.0f, minSide * 0.5f)));
            sf::FloatRect b = txtNum.getLocalBounds();
            txtNum.setOrigin({b.position.x + b.size.x / 2.0f, b.position.y + b.size.y / 2.0f});
            txtNum.setPosition(Rect.getPosition() + Rect.getSize() / 2.0f);

            if (b.size.x < drawW && b.size.y < drawH) window.draw(txtNum);
        }

        window.draw(infoText);
        window.display();

        string fn = dir + "/" + IOD.fileName + "_bin" + to_string(binIdx, 2) + ext;
        window.getTexture().copyToImage().saveToFile(fn);
    }
}
}