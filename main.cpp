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
const int DATASET_SIZE = 1000;
const bool TRAINING_MODE = true;

int main() {
    omp_set_num_threads(16);
    // Konfiguracja sieci neuronowej
    nnutils::FFN::Config ffnConfig;
    // Konfiguracja Heurystyki
    BinpackConstructionHeuristic<nnutils::FFN>::ConfigType heuristicConfig;
    BinpackConstructionHeuristic<nnutils::FFN> heuristic(heuristicConfig);
    // Konfiguracja Algorytmu Ewolucyjnego
    EvoParams evoParams;
    evoParams.populationSize = 500;
    evoParams.generations = 5000;
    evoParams.batchSize = 100;
    evoParams.mutationSigma = 0.2;
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

    // Uczenie heurystyki za pomocą algorytmu ewolucyjnego
    EvolutionaryAlgorithm ea(evoParams, heuristic, datasets);

    if (TRAINING_MODE) {
        ea.run();

        // Zapisanie wag najlepszych sieci do plików
        namespace fs = std::filesystem;
        const std::string MODELS_DIR = "../best_models_specialists/";
        fs::create_directories(MODELS_DIR);

        auto population = ea.getPopulation();
        std::set<std::vector<double>> uniqueGenomes;
        int savedCount = 0;
        for (const auto& ind : population) {
            if (uniqueGenomes.find(ind.genes) == uniqueGenomes.end()) {
                uniqueGenomes.insert(ind.genes);

                nnutils::FFN tempNet(ffnConfig);
                tempNet.setParams(ind.genes.data(), ind.genes.size());

                std::string name = "specialist_" + std::to_string(savedCount);
                tempNet.save(MODELS_DIR, name);
                savedCount++;
            }
        }
        std::cout << "Saved " << savedCount << " unique network weights." << std::endl;

        // Rysowanie wynikow i tworzenie tabeli
        auto population_final = ea.getPopulation();
        vector<vector<double>> allWeights;
        for(const auto& ind : population_final) {
            allWeights.push_back(ind.genes);
        }
        BinDrawer drawer;
        drawer.print_specialist_results(datasets, allWeights, heuristic, "../solutions_specialists", DRAW_ALL_SOLUTIONS);
    }
    else {
        // TO DO
        // Wczytanie wag z pliku
        // nnutils::FFN tempNet(ffnConfig);
        // if (!tempNet.load(MODEL_SAVE_PATH)) {
        //     std::cerr << "Error: Could not load model from " << MODEL_SAVE_PATH << std::endl;
        //     return 1;
        // }
        // // Pobranie wag do heurystyki
        // bestWeights.resize(tempNet.getParamsSize());
        // tempNet.getParams(bestWeights.data(), bestWeights.size());
        // heuristic.setParams(bestWeights.data(), bestWeights.size());
        // // Zapisanie rozwiązań do plików
        // BinDrawer drawer;
        // drawer.print_solutions(datasets, heuristic, "../solutions", DRAW_ALL_SOLUTIONS);
    }
    return 0;
}