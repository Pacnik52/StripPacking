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
#include <numeric>
#include <cmath>

bool PRINT_SOLUTION = true;
bool DRAW_SOLUTION = true;

float

int main() {
    using namespace binpack;

    std::cout << "Loading data..." << std::endl;
    std::vector<BinpackData> datasets;
    DataLoaderOdp loader("ODPS_data_10_1-5_1", true, 0, 10);
    loader.load(datasets);
    if (datasets.empty()) {
        std::cerr << "Error: No datasets were loaded. Check the file path and format." << std::endl;
        return 1;
    }

    std::vector<double> heights;
    int bestIdx = -1, worstIdx = -1;
    double bestVal = std::numeric_limits<double>::max();
    double worstVal = std::numeric_limits<double>::lowest();

    BinDrawer drawer;
    std::string outputDir = "../solutions";
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }

    for (size_t i = 0; i < datasets.size(); ++i) {
        BinpackData& problemData = datasets[i];
        std::vector<BinpackData::BoxType> boxesToPlace;
        if (!problemData.BoxToLoad.empty()) {
            for(size_t j = 0; j < problemData.BoxToLoad.size(); ++j) {
                for(int k = 0; k < problemData.BoxToLoad[j]; ++k) {
                    boxesToPlace.push_back(problemData.BoxTypes[j]);
                }
            }
        } else {
            boxesToPlace = problemData.BoxTypes;
        }

    // auto strategy = std::make_unique<BottomLeftStrategy>();
    // auto strategy = std::make_unique<MaxRectsStrategy>();

    EvolutionaryStrategy::Params eaParams;
    eaParams.populationSize = 50;   // Larger pop = better search, slower
    eaParams.generations = 50;      // More gens = better convergence
    eaParams.mutationRate = 0.3;    // Probability of swapping boxes

    auto strategy = std::make_unique<EvolutionaryStrategy>(eaParams);

    // std::cout << "Starting packing process..." << std::endl;
    StripPacker packer(problemData, std::move(strategy));
    packer.pack(boxesToPlace);

        double height = problemData.getObj();
        heights.push_back(height);
        if (height < bestVal) { bestVal = height; bestIdx = i; }
        if (height > worstVal) { worstVal = height; worstIdx = i; }

        if (PRINT_SOLUTION) {
            std::cout << "Problem " << i << ": Packing complete. Height: " << height << std::endl;
            if (problemData.Solution.BPV.empty()) {
                std::cout << "No boxes were placed." << std::endl;
            } else {
                // std::cout << "Placed " << problemData.Solution.BPV.size() << " boxes:" << std::endl;
                for (const auto& placedBox : problemData.Solution.BPV) {
                    int boxTypeIdx = placedBox.first;
                    const auto& pos = placedBox.second;
                    const auto& boxType = problemData.BoxTypes[boxTypeIdx];
                    // std::cout << "  - Box type " << boxTypeIdx << " (" << boxType.SizeX << "x" << boxType.SizeY
                    //           << ") at (" << pos.X << ", " << pos.Y << ")"
                    //           << (pos.Rotated ? " [rotated]" : "") << std::endl;
                }
            }
        }
    }

    // Statystyki
    double sum = std::accumulate(heights.begin(), heights.end(), 0.0);
    double mean = sum / heights.size();
    double sq_sum = std::inner_product(heights.begin(), heights.end(), heights.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / heights.size() - mean * mean);
    std::cout << "\nSTATS:" << std::endl;
    std::cout << "Avg: " << mean << std::endl;
    std::cout << "Std: " << stdev << std::endl;
    std::cout << "Min: " << bestVal << " (problem " << bestIdx << ")" << std::endl;
    std::cout << "Max: " << worstVal << " (problem " << worstIdx << ")" << std::endl;

    // Zapisz najlepsze i najgorsze rozwiązanie do jednego pliku (np. best_worst_solution.png)
    if (DRAW_SOLUTION && bestIdx != -1 && worstIdx != -1) {
        drawer.drawToFile(datasets[bestIdx], false, outputDir, "_best.png");
        drawer.drawToFile(datasets[worstIdx], false, outputDir, "_worst.png");
        std::cout << "Najlepsze i najgorsze rozwiazanie zapisane do plikow: " << std::endl;
    }
    return 0;
}
