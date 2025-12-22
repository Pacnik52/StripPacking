#include <iostream>
#include <vector>
#include <memory>
#include "DataLoaderOdp.h"
#include "BinpackData.h"
#include "StripPacker.h"
#include "placement_strategies/BottomLeftStrategy.h"
#include "bin_drawer/BinDrawer.h"
#include "placement_strategies/MaxRectsStrategy.h"
#include "placement_strategies/EvolutionaryStrategy.h"

bool PRINT_SOLUTION = true;
bool DRAW_SOLUTION = true;

int main() {
    using namespace binpack;

    std::cout << "Loading data..." << std::endl;
    std::vector<BinpackData> datasets;
    DataLoaderOdp loader("ODPS_small_test", true, 0, 1);
    loader.load(datasets);
    if (datasets.empty()) {
        std::cerr << "Error: No datasets were loaded. Check the file path and format." << std::endl;
        return 1;
    }

    BinpackData& problemData = datasets[0];

    std::vector<BinpackData::BoxType> boxesToPlace;
    if (!problemData.BoxToLoad.empty()) {
        for(size_t i = 0; i < problemData.BoxToLoad.size(); ++i) {
            for(int j = 0; j < problemData.BoxToLoad[i]; ++j) {
                boxesToPlace.push_back(problemData.BoxTypes[i]);
            }
        }
    } else {
        boxesToPlace = problemData.BoxTypes;
    }


    std::cout << "Data loaded. Number of boxes to place: " << boxesToPlace.size() << std::endl;
    std::cout << "Bin dimensions: " << problemData.PSizeX << " x " << problemData.PSizeY << std::endl;

    // auto strategy = std::make_unique<BottomLeftStrategy>();
    // auto strategy = std::make_unique<MaxRectsStrategy>();

    EvolutionaryStrategy::Params eaParams;
    eaParams.populationSize = 50;   // Larger pop = better search, slower
    eaParams.generations = 50;      // More gens = better convergence
    eaParams.mutationRate = 0.3;    // Probability of swapping boxes

    auto strategy = std::make_unique<EvolutionaryStrategy>(eaParams);

    std::cout << "Starting packing process..." << std::endl;
    StripPacker packer(problemData, std::move(strategy));
    packer.pack(boxesToPlace);

    if (PRINT_SOLUTION == true) {
        std::cout << "Packing complete." << std::endl;
        if (problemData.Solution.BPV.empty()) {
            std::cout << "No boxes were placed." << std::endl;
        } else {
            std::cout << "Placed " << problemData.Solution.BPV.size() << " boxes:" << std::endl;
            for (const auto& placedBox : problemData.Solution.BPV) {
                int boxTypeIdx = placedBox.first;
                const auto& pos = placedBox.second;
                const auto& boxType = problemData.BoxTypes[boxTypeIdx];
                std::cout << "  - Box type " << boxTypeIdx << " (" << boxType.SizeX << "x" << boxType.SizeY
                          << ") at (" << pos.X << ", " << pos.Y << ")"
                          << (pos.Rotated ? " [rotated]" : "") << std::endl;
            }
        }
    }

    if (DRAW_SOLUTION == true) {
        std::cout << "Generating solution image..." << std::endl;

        std::string outputDir = "../solutions";
        if (!std::filesystem::exists(outputDir)) {
            std::filesystem::create_directory(outputDir);
        }

        BinDrawer drawer;

        drawer.drawToFile(problemData, false, outputDir, ".png");

        std::cout << "Solution saved to " << outputDir << "/" << problemData.fileName << "_bin00.png" << std::endl;
    }
    return 0;
}
