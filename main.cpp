#include <iostream>
#include <vector>
#include <memory>
#include "DataLoaderOdp.h"
#include "BinpackData.h"
#include "StripPacker.h"
#include "BottomLeftStrategy.h"
#include "BinDrawer.h"

bool PRINT_SOLUTION = true;
bool DRAW_SOLUTION = true;

int main() {
    using namespace binpack;

    // 1. Load the data
    std::cout << "Loading data..." << std::endl;
    // Note: The DataLoaderOdp seems designed to load multiple datasets. We'll use just one for this example.
    std::vector<BinpackData> datasets;
    // The 'odp' parameter seems to be a flag for a specific data format variant.
    // Let's assume 'false' for now. We might need to inspect DataLoaderOdp.cpp to be sure.
    DataLoaderOdp loader("../data/ODPS_small_test", true, 0, 1);
    try {
        loader.load(datasets);
        if (datasets.empty()) {
            std::cerr << "Error: No datasets were loaded. Check the file path and format." << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "An exception occurred during data loading: " << e.what() << std::endl;
        // The loader might throw an error if the file is not found, so we'll stub the data for now.
        std::cerr << "Using stub data because loading failed." << std::endl;
        datasets.emplace_back(20, 1000); // Example: 20 width, 1000 height for the strip
        datasets[0].BoxTypes.push_back({0, 5, 5});
        datasets[0].BoxTypes.push_back({1, 5, 5});
        datasets[0].BoxTypes.push_back({2, 10, 10});
        datasets[0].BoxTypes.push_back({3, 8, 8});
    }

    BinpackData& problemData = datasets[0];

    // Create a vector of all boxes to be placed. The current data structure seems to imply
    // a count for each type, but the strategy expects a simple list of boxes. We'll prepare that list.
    std::vector<BinpackData::BoxType> boxesToPlace;
    if (!problemData.BoxToLoad.empty()) {
        for(size_t i = 0; i < problemData.BoxToLoad.size(); ++i) {
            for(int j = 0; j < problemData.BoxToLoad[i]; ++j) {
                boxesToPlace.push_back(problemData.BoxTypes[i]);
            }
        }
    } else {
        // If BoxToLoad is empty, assume we place one of each type from BoxTypes for demonstration
        boxesToPlace = problemData.BoxTypes;
    }


    std::cout << "Data loaded. Number of boxes to place: " << boxesToPlace.size() << std::endl;
    std::cout << "Bin dimensions: " << problemData.PSizeX << " x " << problemData.PSizeY << std::endl;

    // 2. Create the placement strategy
    auto strategy = std::make_unique<BottomLeftStrategy>();

    // 3. Create the packer and run it
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
