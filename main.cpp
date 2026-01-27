#include <iostream>
#include <vector>
#include <string>
#include <filesystem> // C++17, do tworzenia folderów
#include <set>
#include "startegy/FFN.h"
#include "startegy/BinpackConstructionHeuristic.h"
#include "startegy/EvolutionaryAlgorithm.h"
#include "bin_reader/DataLoaderOdp.h"
#include "bin/BinpackData.h"
#include "bin_drawer/BinDrawer.h"

using namespace binpack;
using namespace std;

const bool DRAW_ALL_SOLUTIONS = true;
const int DATASET_SIZE = 10;
const bool TRAINING_MODE = false;
const bool SPECIALIST_EVOLUTION = false;

void specialist_evolution(EvoParams evoParams, BinpackConstructionHeuristic<nnutils::FFN>& heuristic, std::vector<BinpackData>& datasets, const nnutils::FFN::Config& ffnConfig) {
    EvolutionaryAlgorithm ea(evoParams, heuristic, datasets);
    if (TRAINING_MODE) {
        // Uczenie heurystyki za pomocą algorytmu ewolucyjnego
        ea.run();

        // Wczytywanie ostatniej populacji sieci
        auto population_final = ea.getPopulation();
        vector<vector<double>> allWeights;
        for(const auto& ind : population_final) {
            allWeights.push_back(ind.genes);
        }
        // Zapisanie wag najlepszych sieci do plików
        nnutils::FFN::save_population("../best_models_specialists/", allWeights, ffnConfig);
        std::cout << "Final population of network weights saved." << std::endl;
        // Rysowanie wynikow i tworzenie tabeli
        BinDrawer drawer;
        drawer.print_specialist_results(datasets, allWeights, heuristic, "../solutions_specialists", DRAW_ALL_SOLUTIONS);
    }
    else {
        // Wczytanie wag sieci z plikow
        vector<vector<double>> allWeights = nnutils::FFN::load_population("../best_models_specialists/");
        if (allWeights.empty()) {
            std::cerr << "Error: Failed to load models or directory is empty." << std::endl;
        }

        // Rysowanie wynikow i tworzenie tabeli
        BinDrawer drawer;
        drawer.print_specialist_results(datasets, allWeights, heuristic, "../solutions_specialists", DRAW_ALL_SOLUTIONS);
    }
}

void normal_evolution(EvoParams evoParams, BinpackConstructionHeuristic<nnutils::FFN>& heuristic, std::vector<BinpackData>& datasets, const nnutils::FFN::Config& ffnConfig) {
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
        if (!tempNet.load("../best_models/best_model")) {
            std::cerr << "Error: Could not load model from " << "../best_models" << std::endl;
        }
        // Pobranie wag do heurystyki
        bestWeights.resize(tempNet.getParamsSize());
        tempNet.getParams(bestWeights.data(), bestWeights.size());
        heuristic.setParams(bestWeights.data(), bestWeights.size());
        // Zapisanie rozwiązań do plików
        BinDrawer drawer;
        drawer.print_solutions(datasets, heuristic, "../solutions", DRAW_ALL_SOLUTIONS);
    }
}

int main() {
    omp_set_num_threads(16);
    // Konfiguracja sieci neuronowej
    nnutils::FFN::Config ffnConfig;
    // Konfiguracja Heurystyki
    BinpackConstructionHeuristic<nnutils::FFN>::ConfigType heuristicConfig;
    BinpackConstructionHeuristic<nnutils::FFN> heuristic(heuristicConfig);
    // Konfiguracja Algorytmu Ewolucyjnego
    EvoParams evoParams;
    evoParams.populationSize = 100;
    evoParams.generations = 100;
    evoParams.batchSize = 5;
    evoParams.mutationSigma = 0.2;
    evoParams.mutationAnnealing = true;
    evoParams.elitism = true;
    evoParams.crossover = true;

    // Loading data from files
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
    if (SPECIALIST_EVOLUTION) {
        specialist_evolution(evoParams, heuristic, datasets, ffnConfig);
    }else {
        normal_evolution(evoParams, heuristic, datasets, ffnConfig);
    }
    return 0;
}