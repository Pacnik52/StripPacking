#include <iostream>
#include <filesystem>
#include <vector>
#include "DataLoaderOdp.h"
#include "BinPackData.h"
#include "bin_drawer/BinDrawer.h"
#include "Trainer.h"
#include "NeuralStrategy.h"
#include "NeuralNetwork.h"

using namespace binpack;

const bool DRAW_SOLUTION = true;
const std::string DATA_FILENAME = "ODPS_data_10_1-5_1";
const int DATASET_SIZE = 10;

const int NR_GENERATIONS = 10;

void print_solutions(std::vector<BinpackData>& datasets, NeuralStrategy& strategy, const std::string& outputDir) {
    BinDrawer drawer;
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }

    std::cout << "\nFinal solutions:" << std::endl;
    double totalFF = 0;
    for (size_t i = 0; i < datasets.size(); ++i) {
        BinpackData& problem = datasets[i];

        double ff = strategy.solve(problem);
        totalFF += ff;

        std::cout << "Problem " << problem.fileName << ": Height = " << problem.getSolution().getObj()
                  << ", Fill Factor = " << ff*100 << "%" << std::endl;

        if (DRAW_SOLUTION) {
            drawer.drawToFile(problem, false, outputDir,".png");
        }
    }
    std::cout << "Average Fill Factor: " << (totalFF / datasets.size()) * 100.0 << "%" << std::endl;
}

int main() {
    // Loading data from file
    std::cout << "Loading data..." << std::endl;
    std::vector<BinpackData> datasets;
    DataLoaderOdp loader(DATA_FILENAME, true, 0, DATASET_SIZE);
    loader.load(datasets);
    if (datasets.empty()) {
        std::cerr << "Error: No datasets loaded!" << std::endl;
        return 1;
    }
    std::cout << "Loaded " << datasets.size() << " instances." << std::endl;

    // Black-Box Optimization
    Trainer trainer;
    trainer.train(datasets, NR_GENERATIONS);

    // Final solution evaluation
    NeuralNetwork bestBrain = trainer.getBestNetwork();
    NeuralStrategy strategy(bestBrain);

    print_solutions(datasets, strategy, "../solutions");
    return 0;
}