#include <random>
#include <SFML/Graphics.hpp>
#include "BinDrawer.h"
#include "../utils.h"
#include <set>
#include <string>
#include <algorithm>

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
        for ( auto &BP: IOD.Solution.BPV) {
            Bins.insert(BP.second.binIdx);
        }

        for ( auto binIdx: Bins ) {
            const float IMG_SIZE = 512.0f; // Use float literal

            float minDim = std::min(IOD.PSizeX, IOD.PSizeY);
            if(minDim == 0) minDim = 1.0f;

            float SCALE = IMG_SIZE / minDim;
            float WIDTH = IOD.PSizeX * SCALE;
            float HEIGHT = IOD.PSizeY * SCALE;
            if (flip) {
                swap(WIDTH, HEIGHT);
            }

            sf::RenderTexture window;
            window.resize(sf::Vector2u(static_cast<unsigned int>(WIDTH + 4), static_cast<unsigned int>(HEIGHT + 4)));

            sf::View view = window.getDefaultView();
            view.setSize(sf::Vector2f(WIDTH + 4, -(HEIGHT + 4)));
            window.setView(view);

            window.clear(sf::Color::Black);

            sf::RectangleShape Background(sf::Vector2f(WIDTH, HEIGHT));
            Background.setFillColor(sf::Color::White);
            Background.setPosition(sf::Vector2f(2.f, 2.f));

            sf::Text text(font);

            text.setString(to_string_with_precision(IOD.getObj()*100, 2));
            text.setCharacterSize( static_cast<unsigned int>(SCALE*70) );
            text.setFillColor(sf::Color::Black);

            text.setPosition(sf::Vector2f(WIDTH - IMG_SIZE/3, IMG_SIZE/100));

            vector<sf::RectangleShape> Rects;
            vector<int> BoxTypes;

            sf::Text txtNum(font);
            txtNum.setCharacterSize( static_cast<unsigned int>(SCALE*50) );
            txtNum.setFillColor(sf::Color::Black);


            for( auto &BP: IOD.Solution.BPV ) {
                auto &B = IOD.BoxTypes[BP.first];
                auto Pos = BP.second;
                if (Pos.binIdx != binIdx) {
                    continue;
                }

                float sx = B.SizeX * SCALE;
                float sy = B.SizeY * SCALE;
                if (Pos.Rotated) swap(sx, sy);

                float PosX = Pos.X * SCALE + 2;
                float PosY = (HEIGHT + 2) - (Pos.Y * SCALE) - sy;

                if (flip) {
                    swap(sx, sy);
                    sy = -sy;
                    swap(PosX, PosY);
                    PosY = HEIGHT - PosY;
                }

                sf::RectangleShape Rect(sf::Vector2f(sx, sy));

                if (B.idx < Colors.size()) {
                    Rect.setFillColor(Colors[B.idx]);
                    Rect.setOutlineColor(sf::Color::Black);
                    Rect.setOutlineThickness(-IMG_SIZE/200);
                }
                Rect.setPosition(sf::Vector2f(PosX, PosY));

                Rects.push_back(Rect);
                BoxTypes.push_back(B.idx);
            }

            window.draw(Background);

            for( size_t i = 0; i < Rects.size(); i++ ) {
                auto &R = Rects[i];
                int bt = BoxTypes[i];
                window.draw(R);

                txtNum.setString(std::to_string(bt));

                auto P = R.getPosition();
                auto S = R.getSize();

                sf::Vector2f NP = P + S / 2.0f;
                txtNum.setPosition(NP);

                 sf::FloatRect bounds = txtNum.getLocalBounds();
                 txtNum.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x / 2, bounds.position.y + bounds.size.y / 2));
                 txtNum.setPosition(NP);

                window.draw(txtNum);
            }

            if (Bins.size() == 1) {
                window.draw(text);
            }

            const sf::Texture& outputTexture = window.getTexture();

            sf::Image output = outputTexture.copyToImage();

            string fn = dir + "/" + IOD.fileName + "_bin" + to_string(binIdx, 2) + ext;

            if (!output.saveToFile(fn)) {
                fprintf(stderr, "Failed to save image to %s\n", fn.c_str());
            }
        }
    }
}