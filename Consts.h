#pragma once
#include <string>
#include <fstream>
#include <iostream>

namespace binpack {
    namespace config {
        // Główne tryby i ścieżki
        inline const bool DRAW_ALL_SOLUTIONS = false;
        inline const int TRAINING_DATASET_SIZE = 500;
        inline const int VALIDATION_DATASET_SIZE = 100;
        inline const bool SPECIALIST_EVOLUTION = true;
        inline const bool TRAINING_MODE = true;
        inline const bool ONLY_SELECTION_MODE = false;
        inline const std::string SELECTION_MODE_WEIGHTS_DIR =
                "../results_specialist/2026.07.02_08.44/weights/weights_all/";
        inline const std::string ONLY_RESULTS_MODE_WEIGHTS_DIR =
                "../results_specialist/2026.07.02_08.44/weights/weights_20/";;

        // Parametry algorytmu ewolucyjnego (EvoParams)
        inline const int POPULATION_SIZE = 100;
        inline const int GENERATIONS = 100;
        inline const int BATCH_SIZE = 10;
        inline const double MUTATION_SIGMA = 0.2;
        inline const bool MUTATION_ANNEALING = true;
        inline const bool ELITISM = true;
        inline const bool CROSSOVER = true;

        inline const double COLLECTION_START_PERCENT = 0.9; // np. 0.9 to zbiór z ostatnich 10% generacji
        inline const int NUM_POPULATIONS_TO_COLLECT = 10; // Ile populacji z tego okna chcemy zebrać

        // Funkcja zapisująca konfigurację do pliku
        inline void saveConfig(const std::string &filepath) {
            std::ofstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "Error: Could not create config file at " << filepath << std::endl;
                return;
            }

            file << "=== APP CONFIG ===" << "\n";
            file << "DRAW_ALL_SOLUTIONS = " << (DRAW_ALL_SOLUTIONS ? "true" : "false") << "\n";
            file << "TRAINING_DATASET_SIZE = " << TRAINING_DATASET_SIZE << "\n";
            file << "VALIDATION_DATASET_SIZE = " << VALIDATION_DATASET_SIZE << "\n";
            file << "SPECIALIST_EVOLUTION = " << (SPECIALIST_EVOLUTION ? "true" : "false") << "\n";
            file << "TRAINING_MODE = " << (TRAINING_MODE ? "true" : "false") << "\n";
            file << "ONLY_SELECTION_MODE = " << (ONLY_SELECTION_MODE ? "true" : "false") << "\n";
            file << "SELECTION_MODE_WEIGHTS_DIR = " << SELECTION_MODE_WEIGHTS_DIR << "\n";
            file << "ONLY_RESULTS_MODE_WEIGHTS_DIR = " << ONLY_RESULTS_MODE_WEIGHTS_DIR << "\n";

            file << "\n=== EVO PARAMS ===" << "\n";
            file << "populationSize = " << POPULATION_SIZE << "\n";
            file << "generations = " << GENERATIONS << "\n";
            file << "batchSize = " << BATCH_SIZE << "\n";
            file << "mutationSigma = " << MUTATION_SIGMA << "\n";
            file << "mutationAnnealing = " << (MUTATION_ANNEALING ? "true" : "false") << "\n";
            file << "elitism = " << (ELITISM ? "true" : "false") << "\n";
            file << "crossover = " << (CROSSOVER ? "true" : "false") << "\n";

            file << "collectionStartPercent = " << COLLECTION_START_PERCENT << "\n";
            file << "numPopulationsToCollect = " << NUM_POPULATIONS_TO_COLLECT << "\n";

            file.close();
            std::cout << "Saved config parameters to " << filepath << std::endl;
        }
    }
}
