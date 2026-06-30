#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <numeric>
#include <algorithm>
#include <omp.h>
#include "startegy/FFN.h"
#include "startegy/BinpackConstructionHeuristic.h"
#include "startegy/EvolutionaryAlgorithm.h"
#include "bin_reader/DataLoaderOdp.h"
#include "bin/BinpackData.h"
#include "bin_drawer/BinDrawer.h"

using namespace binpack;
using namespace std;

const bool DRAW_ALL_SOLUTIONS = false;
const int TRAINING_DATASET_SIZE = 100;
const int VALIDATION_DATASET_SIZE = 100;
const int VALIDATION_INTERATIONS = 10;
const bool SPECIALIST_EVOLUTION = true;
const bool TRAINING_MODE = true;
const bool ONLY_SELECTION_MODE = true;
const std::string SPECIALIST_WEIGHTS_DIR = "../best_models_specialists_whole_final_1/";


std::string getCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y.%m.%d_%H.%M");
    return ss.str();
}

void select_and_save_specialists(EvolutionaryAlgorithm &ea,
                                 const std::vector<EvolutionaryAlgorithm::Individual> &candidatePool,
                                 const std::vector<std::vector<double> > &scoreMatrix,
                                 std::vector<BinpackData> &testSet,
                                 int subsetSize,
                                 const std::string resultsDir,
                                 const std::string weightsDir,
                                 BinpackConstructionHeuristic<nnutils::FFN> &heuristic,
                                 const nnutils::FFN::Config &ffnConfig) {
    std::cout << "\n--- Selecting " << subsetSize << " specialists ---" << std::endl;
    auto specialists = ea.selectSpecialistsFromMatrix(candidatePool, scoreMatrix, subsetSize);
    if (specialists.empty()) {
        std::cerr << "Error: Could not select specialists for size " << subsetSize << std::endl;
        return;
    }

    std::vector<std::vector<double> > specialistWeights;
    specialistWeights.reserve(specialists.size());
    for (const auto &ind: specialists) {
        specialistWeights.push_back(ind.genes);
    }

    std::string sizeStr = std::to_string(subsetSize);
    std::string specialistWeightsDir = weightsDir + "/weights_" + sizeStr;
    std::filesystem::create_directories(specialistWeightsDir);

    nnutils::FFN::save_population(specialistWeightsDir, specialistWeights, ffnConfig);
    std::cout << "Saved " << specialists.size() << " network models to " << specialistWeightsDir << std::endl;
    BinDrawer drawer;
    drawer.print_specialist_results(testSet, specialistWeights, heuristic, resultsDir, sizeStr,
                                    DRAW_ALL_SOLUTIONS);
    std::cout << "Saved results to " << resultsDir << std::endl;
}

void only_selection_mode(EvoParams evoParams, BinpackConstructionHeuristic<nnutils::FFN> &heuristic,
                         std::vector<BinpackData> &trainingSet, const nnutils::FFN::Config &ffnConfig,
                         std::vector<BinpackData> &validationSet) {
    EvolutionaryAlgorithm ea(evoParams, heuristic, trainingSet, validationSet);

    vector<vector<double> > allWeights = nnutils::FFN::load_population(SPECIALIST_WEIGHTS_DIR);
    if (allWeights.empty()) {
        std::cerr << "Error: Failed to load models from " << SPECIALIST_WEIGHTS_DIR << std::endl;
        return;
    }

    std::vector<EvolutionaryAlgorithm::Individual> candidatePool;
    candidatePool.reserve(allWeights.size());
    for (size_t i = 0; i < allWeights.size(); ++i) {
        EvolutionaryAlgorithm::Individual ind;
        ind.genes = allWeights[i];
        ind.id = i;
        candidatePool.push_back(ind);
    }

    // Przygotowanie folderów
    std::string baseDir = "../results_specialist_" + getCurrentDateString();
    std::string resultsDir = baseDir + "/results";
    std::string weightsDir = baseDir + "/weights";
    std::filesystem::create_directories(resultsDir);
    std::filesystem::create_directories(weightsDir);

    std::cout << "Saving ALL loaded weights and calculating their global results..." << std::endl;
    nnutils::FFN::save_population(weightsDir + "/weights_all", allWeights, ffnConfig);
    BinDrawer drawer;
    auto scoreMatrix = drawer.print_specialist_results(validationSet, allWeights, heuristic, resultsDir, "all",
                                                       DRAW_ALL_SOLUTIONS);

    std::vector<int> subsetSizes = {5, 10, 20, 50, 100};
    for (int size: subsetSizes) {
        if (size > candidatePool.size()) {
            std::cout << "Skipping selection for size " << size << ". Not enough networks in candidate pool." <<
                    std::endl;
            continue;
        }
        select_and_save_specialists(ea, candidatePool, scoreMatrix, validationSet, size, resultsDir, weightsDir,
                                    heuristic,
                                    ffnConfig);
    }
}

void specialist_evolution(EvoParams evoParams, BinpackConstructionHeuristic<nnutils::FFN> &heuristic,
                          std::vector<BinpackData> &trainingSet, const nnutils::FFN::Config &ffnConfig,
                          std::vector<BinpackData> &validationSet) {
    EvolutionaryAlgorithm ea(evoParams, heuristic, trainingSet, validationSet);

    std::string baseDir = "../results_specialist_" + getCurrentDateString();
    std::string resultsDir = baseDir + "/results";
    std::string weightsDir = baseDir + "/weights";

    if (TRAINING_MODE) {
        ea.run();

        std::cout << "\n=== FINAL EVALUATION ON VALIDATION SET ===" << std::endl;
        auto collectedPopulations = ea.getFinalPopulations();
        std::cout << "Total collected individuals from final generations: " << collectedPopulations.size() << std::endl;

        std::vector<std::vector<double> > allWeightsBeforeGreed;
        for (const auto &ind: collectedPopulations) {
            allWeightsBeforeGreed.push_back(ind.genes);
        }

        std::string allWeightsDir = baseDir + "/all_weights";
        std::filesystem::create_directories(allWeightsDir);

        nnutils::FFN::save_population(allWeightsDir, allWeightsBeforeGreed, ffnConfig);
        std::cout << "Final population of network weights saved to " << allWeightsDir << std::endl;

        if (!collectedPopulations.empty() && !validationSet.empty()) {
            std::string specialistsBaseDir = baseDir + "/specialists";
            auto scoreMatrix = ea.buildScoreMatrix(collectedPopulations, validationSet);
            select_and_save_specialists(ea, collectedPopulations, scoreMatrix, trainingSet, evoParams.specialistSetSize,
                                        resultsDir, weightsDir, heuristic, ffnConfig);
        } else {
            std::cerr << "Error: No specialists collected or validation set is empty." << std::endl;
        }
    } else {
        std::string weightsDir = baseDir + "/specialists/specialists_weights_" + std::to_string(
                                     evoParams.specialistSetSize);
        std::vector<std::vector<double> > allWeights = nnutils::FFN::load_population(weightsDir);
        if (allWeights.empty()) {
            std::cerr << "Error: Failed to load models from " << weightsDir << std::endl;
            return;
        }
        std::string resultsDir = baseDir + "/specialists/specialists_results_" + std::to_string(
                                     evoParams.specialistSetSize);
        std::filesystem::create_directories(resultsDir);

        BinDrawer drawer;
        drawer.print_specialist_results(trainingSet, allWeights, heuristic, resultsDir, "all", DRAW_ALL_SOLUTIONS);
    }
}

void normal_evolution(EvoParams evoParams, BinpackConstructionHeuristic<nnutils::FFN> &heuristic,
                      std::vector<BinpackData> &trainingSet, const nnutils::FFN::Config &ffnConfig,
                      std::vector<BinpackData> &validationSet) {
    EvolutionaryAlgorithm ea(evoParams, heuristic, trainingSet, validationSet);
    vector<double> bestWeights;
    if (TRAINING_MODE) {
        ea.run_normal();
        bestWeights = ea.getBestWeights();
        heuristic.setParams(bestWeights.data(), bestWeights.size());
        cout << "Training finished. Best weights found." << endl;
        BinDrawer drawer;
        drawer.print_solutions(trainingSet, heuristic, "../solutions", DRAW_ALL_SOLUTIONS);

        nnutils::FFN tempNet(ffnConfig);
        tempNet.setParams(bestWeights.data(), bestWeights.size());
        tempNet.save("../best_models", "best_model");
    } else {
        nnutils::FFN tempNet(ffnConfig);
        if (!tempNet.load("../best_models/new_best_best_model")) {
            std::cerr << "Error: Could not load model from " << "../best_models" << std::endl;
        }
        bestWeights.resize(tempNet.getParamsSize());
        tempNet.getParams(bestWeights.data(), bestWeights.size());
        heuristic.setParams(bestWeights.data(), bestWeights.size());
        BinDrawer drawer;
        drawer.print_solutions(trainingSet, heuristic, "../solutions", DRAW_ALL_SOLUTIONS);
    }
}

int main() {
    omp_set_num_threads(16);
    nnutils::FFN::Config ffnConfig;
    BinpackConstructionHeuristic<nnutils::FFN>::ConfigType heuristicConfig;
    BinpackConstructionHeuristic<nnutils::FFN> heuristic(heuristicConfig);

    EvoParams evoParams;
    evoParams.populationSize = 500;
    evoParams.generations = 100;
    evoParams.batchSize = 100;
    evoParams.mutationSigma = 0.2;
    evoParams.mutationAnnealing = true;
    evoParams.elitism = true;
    evoParams.crossover = false;
    evoParams.validationCheckInterval = 10;
    evoParams.finalEvaluationWindow = 100;
    evoParams.specialistSetSize = 100;

    std::cout << "Loading data..." << std::endl;
    std::vector<BinpackData> trainingDataset;
    std::vector<BinpackData> validationSet;
    std::vector<std::string> filenames = {
        "ODPS_data_10_1-5_1",
        "ODPS_data_10_1-5_2",
        "ODPS_data_10_1-5_6",
        "ODPS_data_10_1-5_16"
    };
    DataLoaderOdp::loadTrainAndValidationFromMultipleFiles(filenames, trainingDataset, TRAINING_DATASET_SIZE,
                                                           validationSet, VALIDATION_DATASET_SIZE, true);

    if (trainingDataset.empty() or validationSet.empty()) {
        std::cerr << "Error: No datasets loaded!" << std::endl;
        return 1;
    }
    std::cout << "Loaded " << trainingDataset.size() << " training instances and " << validationSet.size() <<
            " validation instances." << std::endl;

    // Podzial trybow pracy programu
    if (ONLY_SELECTION_MODE) {
        only_selection_mode(evoParams, heuristic, trainingDataset, ffnConfig, validationSet);
    } else if (SPECIALIST_EVOLUTION) {
        specialist_evolution(evoParams, heuristic, trainingDataset, ffnConfig, validationSet);
    } else {
        normal_evolution(evoParams, heuristic, trainingDataset, ffnConfig, validationSet);
    }

    return 0;
};
