#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <filesystem>
#include "BinpackData.h"

namespace binpack {
    using namespace std;

    class BinDrawer {
    public:
        vector<sf::Color> Colors;
        sf::Font font;

        BinDrawer() {
            populateColors();

            // SFML 3 FIX: loadFromFile -> openFromFile
            if (!font.openFromFile("../fnt/arial.ttf"))
            {
                // Optional: Print error if font fails
                // puts("Error loading font!");
            }
        }

        void populateColors();

        void drawToFile(const BinpackData &IOD, bool flip, string dir, string ext);
    };
}