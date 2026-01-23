#include <iostream>
#include <vector>
#include "startegy/FFN.h"
#include "startegy/BinpackConstructionHeuristic.h"
#include "startegy/EvolutionaryAlgorithm.h"
#include "bin_reader/DataLoaderOdp.h"
#include "bin/BinpackData.h"
#include "bin_drawer/BinDrawer.h"

using namespace binpack;
using namespace std;

const bool DRAW_ALL_SOLUTIONS = true;
const std::string DATA_FILENAME = "ODPS_data_10_1-5_1";
const int DATASET_SIZE = 10;



int main() {
    // Konfiguracja sieci neuronowej
    nnutils::FFN::Config ffnConfig;
    // Konfiguracja Heurystyki
    BinpackConstructionHeuristic<nnutils::FFN>::ConfigType heuristicConfig;
    BinpackConstructionHeuristic<nnutils::FFN> heuristic(heuristicConfig);
    // Konfiguracja Algorytmu Ewolucyjnego
    EvoParams evoParams;
    evoParams.populationSize = 10;
    evoParams.generations = 2;
    evoParams.batchSize = 10;
    evoParams.mutationSigma = 0.2;

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

    // Uczenie heurystyki za pomocą algorytmu ewolucyjnego
    EvolutionaryAlgorithm ea(evoParams, heuristic, datasets);
    ea.run();

    // Pobranie i testowanie najlepszej sieci
    vector<double> bestWeights = ea.getBestWeights();
    heuristic.setParams(bestWeights.data(), bestWeights.size());
    cout << "Training finished. Best weights found." << endl;
    BinDrawer drawer;
    drawer.print_solutions(datasets, heuristic, "../solutions", DRAW_ALL_SOLUTIONS);

    // Zapisanie najlepszego modelu do pliku
    nnutils::FFN tempNet(ffnConfig);
    tempNet.setParams(bestWeights.data(), bestWeights.size());
    tempNet.save("best_model.weights");

    return 0;
}