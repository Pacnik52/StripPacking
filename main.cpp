#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "Consts.h"
#include "startegy/FFN.h"
#include "startegy/BinpackConstructionHeuristic.h"
#include "startegy/EvolutionaryAlgorithm.h"
#include "bin_reader/DataLoaderOdp.h"
#include "bin/BinpackData.h"
#include "bin_drawer/BinDrawer.h"

using namespace binpack;
using namespace std;

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
                                    config::DRAW_ALL_SOLUTIONS);
    std::cout << "Saved results to " << resultsDir << std::endl;
}

void only_selection_mode(EvoParams evoParams, BinpackConstructionHeuristic<nnutils::FFN> &heuristic,
                         std::vector<BinpackData> &trainingSet, const nnutils::FFN::Config &ffnConfig,
                         std::vector<BinpackData> &validationSet) {
    EvolutionaryAlgorithm ea(evoParams, heuristic, trainingSet, validationSet);

    vector<vector<double> > allWeights = nnutils::FFN::load_population(config::SELECTION_MODE_WEIGHTS_DIR);
    if (allWeights.empty()) {
        std::cerr << "Error: Failed to load models from " << config::SELECTION_MODE_WEIGHTS_DIR << std::endl;
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
    std::string dateStr = getCurrentDateString();
    std::string baseDir = "../results_specialist_" + dateStr;
    std::string resultsDir = baseDir + "/results";
    std::string weightsDir = baseDir + "/weights";
    std::filesystem::create_directories(resultsDir);
    std::filesystem::create_directories(weightsDir);

    std::cout << "Saving ALL loaded weights and calculating their global results..." << std::endl;
    nnutils::FFN::save_population(weightsDir + "/weights_all", allWeights, ffnConfig);
    BinDrawer drawer;
    std::cout << "Creating score matrix..." << std::endl;
    auto scoreMatrix = drawer.print_specialist_results(validationSet, allWeights, heuristic, resultsDir, "all",
                                                       config::DRAW_ALL_SOLUTIONS);

    std::vector<int> subsetSizes = {5, 10, 20, 50, 100};
    for (int size: subsetSizes) {
        if (size > allWeights.size()) {
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

    std::vector<std::vector<double> > allWeights;
    if (config::TRAINING_MODE) {
        ea.run();
        // get final populations
        auto collectedPopulations = ea.getFinalPopulations();
        std::cout << "Total collected individuals from final generations: " << collectedPopulations.size() << std::endl;
        for (const auto &ind: collectedPopulations) {
            allWeights.push_back(ind.genes);
        }

        // Przygotowanie folderów
        std::string dateStr = getCurrentDateString();
        std::string baseDir = "../results_specialist/" + dateStr;
        std::string resultsDir = baseDir + "/results";
        std::string weightsDir = baseDir + "/weights";
        std::filesystem::create_directories(resultsDir);
        std::filesystem::create_directories(weightsDir);

        std::cout << "Saving ALL loaded weights and calculating their global results..." << std::endl;
        config::saveConfig(baseDir + "/params_" + dateStr + ".txt");
        nnutils::FFN::save_population(weightsDir + "/weights_all", allWeights, ffnConfig);
        BinDrawer drawer;
        auto scoreMatrix = drawer.print_specialist_results(validationSet, allWeights, heuristic, resultsDir, "all",
                                                           config::DRAW_ALL_SOLUTIONS);

        std::vector<int> subsetSizes = {5, 10, 20, 50, 100};
        for (int size: subsetSizes) {
            if (size > collectedPopulations.size()) {
                std::cout << "Skipping selection for size " << size << ". Not enough networks in candidate pool." <<
                        std::endl;
                continue;
            }
            select_and_save_specialists(ea, collectedPopulations, scoreMatrix, validationSet, size, resultsDir,
                                        weightsDir,
                                        heuristic,
                                        ffnConfig);
        }
    } else {
        // Przygotowanie folderów
        std::string dateStr = getCurrentDateString();
        std::string baseDir = "../results_specialist/" + dateStr;
        std::string resultsDir = baseDir + "/results";
        std::filesystem::create_directories(resultsDir);

        allWeights = nnutils::FFN::load_population(config::ONLY_RESULTS_MODE_WEIGHTS_DIR);
        if (allWeights.empty()) {
            std::cerr << "Error: Failed to load models from " << config::ONLY_RESULTS_MODE_WEIGHTS_DIR << std::endl;
            return;
        }

        BinDrawer drawer;
        drawer.print_specialist_results(validationSet, allWeights, heuristic, resultsDir, "all",
                                        config::DRAW_ALL_SOLUTIONS);
    }
}

void normal_evolution(EvoParams evoParams, BinpackConstructionHeuristic<nnutils::FFN> &heuristic,
                      std::vector<BinpackData> &trainingSet, const nnutils::FFN::Config &ffnConfig,
                      std::vector<BinpackData> &validationSet) {
    EvolutionaryAlgorithm ea(evoParams, heuristic, trainingSet, validationSet);
    vector<double> bestWeights;
    std::vector<std::vector<double> > allWeights;
    if (config::TRAINING_MODE) {
        ea.run_normal();
        auto collectedPopulations = ea.getFinalPopulations();
        std::cout << "Total collected individuals from final generations: " << collectedPopulations.size() << std::endl;
        for (const auto &ind: collectedPopulations) {
            allWeights.push_back(ind.genes);
        }
        // Przygotowanie folderów
        std::string dateStr = getCurrentDateString();
        std::string baseDir = "../results_normal/" + dateStr;
        std::string resultsDir = baseDir + "/results";
        std::string weightsDir = baseDir + "/weights";
        std::filesystem::create_directories(resultsDir);
        std::filesystem::create_directories(weightsDir);

        std::cout << "Saving ALL loaded weights and calculating their global results..." << std::endl;
        config::saveConfig(baseDir + "/params_" + dateStr + ".txt");
        nnutils::FFN::save_population(weightsDir + "/weights_all", allWeights, ffnConfig);

        std::cout << "Saving best weights and calculating global results..." << std::endl;
        bestWeights = ea.getBestWeightsFromValidation();
        heuristic.setParams(bestWeights.data(), bestWeights.size());
        BinDrawer drawer;
        drawer.print_solutions(validationSet, heuristic, resultsDir, config::DRAW_ALL_SOLUTIONS);
        nnutils::FFN tempNet(ffnConfig);
        tempNet.setParams(bestWeights.data(), bestWeights.size());
        tempNet.save(weightsDir, "best_model");
    } else {
        // Przygotowanie folderów
        std::string dateStr = getCurrentDateString();
        std::string baseDir = "../results_normal/" + dateStr;
        std::string resultsDir = baseDir + "/results";
        std::filesystem::create_directories(resultsDir);

        nnutils::FFN tempNet(ffnConfig);
        if (!tempNet.load("../best_models/new_best_best_model")) {
            std::cerr << "Error: Could not load model from " << "../best_models" << std::endl;
        }
        bestWeights.resize(tempNet.getParamsSize());
        tempNet.getParams(bestWeights.data(), bestWeights.size());
        heuristic.setParams(bestWeights.data(), bestWeights.size());
        BinDrawer drawer;
        drawer.print_solutions(trainingSet, heuristic, resultsDir, config::DRAW_ALL_SOLUTIONS);
    }
}

int main() {
    nnutils::FFN::Config ffnConfig;
    BinpackConstructionHeuristic<nnutils::FFN>::ConfigType heuristicConfig;
    BinpackConstructionHeuristic<nnutils::FFN> heuristic(heuristicConfig);

    EvoParams evoParams;
    evoParams.populationSize = config::POPULATION_SIZE;
    evoParams.generations = config::GENERATIONS;
    evoParams.batchSize = config::BATCH_SIZE;
    evoParams.mutationSigma = config::MUTATION_SIGMA;
    evoParams.mutationRate = config::MUTATION_RATE;
    evoParams.mutationAnnealing = config::MUTATION_ANNEALING;
    evoParams.elitism = config::ELITISM;
    evoParams.crossover = config::CROSSOVER;
    evoParams.collectionStartPercent = config::COLLECTION_START_PERCENT;
    evoParams.numPopulationsToCollect = config::NUM_POPULATIONS_TO_COLLECT;

    std::cout << "Loading data..." << std::endl;
    std::vector<BinpackData> trainingDataset;
    std::vector<BinpackData> validationSet;
    std::vector<std::string> filenames = {
        "SPP_1",
        "SPP_2",
        "SPP_3",
        "SPP_4",
        "SPP_5",
    };
    DataLoaderOdp::loadTrainAndValidationFromMultipleFiles(filenames, trainingDataset, config::TRAINING_DATASET_SIZE,
                                                           validationSet, config::VALIDATION_DATASET_SIZE, true);

    if (trainingDataset.empty() or validationSet.empty()) {
        std::cerr << "Error: No datasets loaded!" << std::endl;
        return 1;
    }
    std::cout << "Loaded " << trainingDataset.size() << " training instances and " << validationSet.size() <<
            " validation instances." << std::endl;

    // Podzial trybow pracy programu
    if (config::ONLY_SELECTION_MODE) {
        only_selection_mode(evoParams, heuristic, trainingDataset, ffnConfig, validationSet);
    } else if (config::SPECIALIST_EVOLUTION) {
        specialist_evolution(evoParams, heuristic, trainingDataset, ffnConfig, validationSet);
    } else {
        normal_evolution(evoParams, heuristic, trainingDataset, ffnConfig, validationSet);
    }

    return 0;
};
