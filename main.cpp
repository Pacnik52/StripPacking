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
const bool TRAINING_MODE = true;
const std::string MODEL_SAVE_PATH = "../best_models/ok_model.weights";

int main() {
    omp_set_num_threads(16);
    // Konfiguracja sieci neuronowej
    nnutils::FFN::Config ffnConfig;
    // Konfiguracja Heurystyki
    BinpackConstructionHeuristic<nnutils::FFN>::ConfigType heuristicConfig;
    BinpackConstructionHeuristic<nnutils::FFN> heuristic(heuristicConfig);
    // Konfiguracja Algorytmu Ewolucyjnego
    EvoParams evoParams;
    evoParams.populationSize = 50;
    evoParams.generations = 100;
    evoParams.batchSize = 10;
    evoParams.mutationSigma = 0.2;

    // Loading data from file
    std::cout << "Loading data..." << std::endl;
    std::vector<BinpackData> datasets;
    std::vector<std::string> filenames = {
        "ODPS_data_10_1-5_1",
        "ODPS_data_10_1-5_2",
        "ODPS_data_10_1-5_6",
        "ODPS_data_10_1-5_16"
    };
    DataLoaderOdp::loadFromMultipleFiles(filenames, DATASET_SIZE, datasets, true);
    if (datasets.empty()) {
        std::cerr << "Error: No datasets loaded!" << std::endl;
        return 1;
    }
    std::cout << "Loaded " << datasets.size() << " instances." << std::endl;

    // Uczenie heurystyki za pomocą algorytmu ewolucyjnego
    EvolutionaryAlgorithm ea(evoParams, heuristic, datasets);
    vector<double> bestWeights;
    if (TRAINING_MODE) {
        ea.run();
        // Zapisanie rozwiązań do plików
        bestWeights = ea.getBestWeights();
        heuristic.setParams(bestWeights.data(), bestWeights.size());
        cout << "Training finished. Best weights found." << endl;
        BinDrawer drawer;
        drawer.print_solutions(datasets, heuristic, "../solutions", DRAW_ALL_SOLUTIONS);

        // Zapisanie najlepszego modelu do pliku
        nnutils::FFN tempNet(ffnConfig);
        tempNet.setParams(bestWeights.data(), bestWeights.size());
        tempNet.save("../best_models","best_model");
    }
    else {
        // Wczytanie wag z pliku
        nnutils::FFN tempNet(ffnConfig);
        if (!tempNet.load(MODEL_SAVE_PATH)) {
            std::cerr << "Error: Could not load model from " << MODEL_SAVE_PATH << std::endl;
            return 1;
        }
        // Pobranie wag do heurystyki
        bestWeights.resize(tempNet.getParamsSize());
        tempNet.getParams(bestWeights.data(), bestWeights.size());
        heuristic.setParams(bestWeights.data(), bestWeights.size());
        // Zapisanie rozwiązań do plików
        BinDrawer drawer;
        drawer.print_solutions(datasets, heuristic, "../solutions", DRAW_ALL_SOLUTIONS);
    }
    return 0;
}