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

const bool DRAW_SOLUTION = true;
const std::string DATA_FILENAME = "ODPS_data_10_1-5_1";
const int DATASET_SIZE = 100;

void print_solutions(std::vector<BinpackData>& datasets,
                     BinpackConstructionHeuristic<nnutils::FFN>& heuristic,
                     const std::string& outputDir) {

    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }

    BinDrawer drawer;
    std::cout << "\nGenerating final solutions and images..." << std::endl;

    double totalFF = 0;

    for (size_t i = 0; i < datasets.size(); ++i) {
        BinpackData& problem = datasets[i];

        auto solution = heuristic.run(problem);

        problem.Solution = solution;

        // 3. Pobierz Fill Factor (zakładamy, że getObj zwraca FF dla StripPacking w tej implementacji)
        double ff = solution.getObj();
        totalFF += ff;

        std::cout << "Instance " << problem.fileName
                  << ": Fill Factor = " << ff * 100.0 << "%" << std::endl;

        // 4. Narysuj rozwiązanie
        if (DRAW_SOLUTION) {
            // Uwaga: filename, false (np. nie tylko kontury), katalog, rozszerzenie
            drawer.drawToFile(problem, false, outputDir, ".png");
        }
    }

    if (!datasets.empty()) {
        std::cout << "Average Fill Factor: " << (totalFF / datasets.size()) * 100.0 << "%" << std::endl;
        std::cout << "Solutions saved to directory: " << outputDir << "/" << std::endl;
    }
}

int main() {
    // 1. Konfiguracja sieci neuronowej
    nnutils::FFN::Config ffnConfig;
    ffnConfig.inputSize = 25; // Dopasowane do BinpackConstructionHeuristic.h
    ffnConfig.hidden1Size = 32;
    ffnConfig.hidden2Size = 12;
    ffnConfig.outputSize = 1;

    // 2. Konfiguracja heurystyki
    BinpackConstructionHeuristic<nnutils::FFN>::ConfigType heuristicConfig;
    heuristicConfig.stripPacking = true; // Rozwiązujemy problem Strip Packing (minimalizacja wysokości)
    heuristicConfig.binPackInt = false;
    heuristicConfig.AConf = ffnConfig;

    // Tworzenie prototypu heurystyki (używany do klonowania w EA)
    BinpackConstructionHeuristic<nnutils::FFN> heuristic(heuristicConfig);

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


    // 4. Konfiguracja Algorytmu Ewolucyjnego
    EvoParams evoParams;
    evoParams.populationSize = 10; // Mniejsza populacja dla szybszych testów
    evoParams.generations = 100;
    evoParams.batchSize = 20;       // Ocena na 20 losowych instancjach w każdej iteracji
    evoParams.mutationSigma = 0.2;  // Początkowa siła mutacji

    // 5. Uruchomienie treningu
    EvolutionaryAlgorithm ea(evoParams, heuristic, datasets);
    ea.run();

    // 6. Pobranie i testowanie najlepszego wyniku
    vector<double> bestWeights = ea.getBestWeights();
    cout << "Training finished. Best weights found." << endl;

    // Ustawienie najlepszych wag w heurystyce
    heuristic.setParams(bestWeights.data(), bestWeights.size());

    // Tutaj można zapisać wagi do pliku lub przetestować na zbiorze walidacyjnym
    nnutils::FFN tempNet(ffnConfig);
    tempNet.setParams(bestWeights.data(), bestWeights.size());
    tempNet.save("best_model.weights");
    print_solutions(datasets, heuristic, "../solutions");

    return 0;
}