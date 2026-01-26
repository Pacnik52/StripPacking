#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <filesystem>
#include "../bin/BinpackData.h"
#include "../startegy/BinpackConstructionHeuristic.h"

namespace binpack {
    using namespace std;

    class BinDrawer {
    public:
        vector<sf::Color> Colors;
        sf::Font font;

        BinDrawer() {
            populateColors();

            if (!font.openFromFile("../bin_drawer/fnt/arial.ttf"))
            {
                std::cerr << "Font file not found." << std::endl;
            }
        }

        void print_solutions(std::vector<BinpackData>& datasets, BinpackConstructionHeuristic<nnutils::FFN>& heuristic, const std::string& outputDir, bool draw_all_solutions );

        void populateColors();

        void print_specialist_results(
            std::vector<BinpackData>& datasets,
            const std::vector<std::vector<double>>& populationGenomes,
            BinpackConstructionHeuristic<nnutils::FFN>& heuristic,
            const std::string& outputDir, bool draw_all_best_solutions
        );

        void drawToFile(const BinpackData &IOD, bool flip, string dir, string ext);
    };
}