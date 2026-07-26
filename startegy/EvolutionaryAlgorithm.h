#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include <set>
#include <cmath>
#include "BinpackConstructionHeuristic.h"
#include "../bin_reader/DataLoaderOdp.h"

namespace binpack {
    struct EvoParams {
        int populationSize = 100;
        int generations = 1000;
        int batchSize = 100;
        double mutationRate = 0.1;
        double mutationSigma = 0.1;
        bool mutationAnnealing = true;
        double crossoverRate = 0.8;
        double minSigma = 0.05;
        double decayFactor = 1.0;
        bool elitism = true;
        int eliteSize = populationSize / 10;
        int tournamentSize = 5;
        bool crossover = false;
        double collectionStartPercent = 0.9;
        int numPopulationsToCollect = 10;
        int specialistSetSize = 10;
    };

    class EvolutionaryAlgorithm {
    public:
        using HeuristicType = BinpackConstructionHeuristic<nnutils::FFN>;
        using Genome = std::vector<double>;

        struct Individual {
            Genome genes;
            double avgFitness;
            int id;
        };

    private:
        EvoParams params;
        HeuristicType heuristicPrototype;
        const std::vector<BinpackData> &allTrainingData;
        const std::vector<BinpackData> &validationData;
        std::mt19937 rng;
        std::vector<Individual> population;
        std::vector<Individual> finalPopulations;
        int nextIndId = 0;

    public:
        EvolutionaryAlgorithm(const EvoParams &_params,
                              const HeuristicType &_heuristic,
                              const std::vector<BinpackData> &_data,
                              const std::vector<BinpackData> &_validation)
            : params(_params), heuristicPrototype(_heuristic), allTrainingData(_data), validationData(_validation) {
            std::random_device rd;
            rng.seed(rd());

            initPopulation();
        }

        void initPopulation() {
            int genomeSize = heuristicPrototype.getParamsSize();
            population.resize(params.populationSize);

            std::normal_distribution<double> dist(0.0, 0.1);

            for (auto &ind: population) {
                ind.genes.resize(genomeSize);
                for (double &gene: ind.genes) {
                    gene = dist(rng);
                }
                ind.avgFitness = -DBL_MAX;
                ind.id = nextIndId++;
            }
        }

        void run() {
            int genomeSize = heuristicPrototype.getParamsSize();
            std::cout << "Starting Specialist Evolution. Genome size: " << genomeSize << std::endl;
            if (params.mutationAnnealing) {
                if (params.mutationSigma > params.minSigma && params.generations > 0) {
                    params.decayFactor = std::pow(params.minSigma / params.mutationSigma, 1.0 / params.generations);
                } else {
                    std::cout << "Stopping early: Mutation Annealing parameters conflict." << std::endl;
                    return;
                }
            }
            std::vector<int> targetGens = calculateTargetGenerations();
            for (int gen = 0; gen < params.generations; ++gen) {
                // Wybór losowego batcha zadań
                std::vector<BinpackData> batch;
                std::sample(allTrainingData.begin(), allTrainingData.end(),
                            std::back_inserter(batch), params.batchSize, rng);

                // Macierz wyników: [Siec][Zadanie] -> wynik (fill factor)
                std::vector<std::vector<double> > scores(params.populationSize, std::vector<double>(batch.size()));

                // Ewaluacja calej populacji sieci dla wszystkich zadań
#pragma omp parallel for schedule(dynamic)
                for (int i = 0; i < params.populationSize; ++i) {
                    HeuristicType localHeuristic = heuristicPrototype;
                    localHeuristic.setParams(population[i].genes.data(), population[i].genes.size());

                    double sumFit = 0.0;
                    for (int j = 0; j < batch.size(); ++j) {
                        auto solution = localHeuristic.run(batch[j]);
                        double val = solution.getObj();
                        scores[i][j] = val;
                        sumFit += val;
                    }
                    population[i].avgFitness = sumFit / batch.size();
                }

                // Print stats
                // double globalBestAvg = -DBL_MAX;
                // for(const auto& ind : population) globalBestAvg = std::max(globalBestAvg, ind.avgFitness);
                // std::cout << "Gen " << gen << " | Max Avg Fitness: " << globalBestAvg << " ..." << std::endl;
                std::cout << "Gen " << gen << "..." << std::endl;

                // Selekcja rodzicow nastepnej generacji - dla kazdego zadania zostaje wybrana najlepsza siec ktora zostaje rodzicem
                std::vector<int> parentIndices;
                parentIndices.reserve(batch.size());

                for (int j = 0; j < batch.size(); ++j) {
                    int winnerIdx = -1;
                    double bestScore = -DBL_MAX;
                    // Znajdowanie najlepszej sieci dla zadania j
                    for (int i = 0; i < params.populationSize; ++i) {
                        if (scores[i][j] > bestScore) {
                            bestScore = scores[i][j];
                            winnerIdx = i;
                        }
                    }
                    if (winnerIdx != -1) {
                        parentIndices.push_back(winnerIdx);
                    }
                }

                // Tworzenie nowej populacji
                std::vector<Individual> newPop;
                newPop.reserve(params.populationSize);

                // Jesli elitaryzm to przenosimy najlepsze rozwiazania do nowej populacji bez mutacji
                if (params.elitism) {
                    std::set<int> uniqueWinners(parentIndices.begin(), parentIndices.end());
                    for (int idx: uniqueWinners) {
                        newPop.push_back(population[idx]);
                    }
                }

                // Uzupełniamy resztę populacji krzyżując losowych rodziców
                std::uniform_int_distribution<int> parentDist(0, parentIndices.size() - 1);

                // Jeśli crossover to krzyzowanie i mutacja, jesli nie to tylko mutacja
                // Rodzice wybierani selekcja losowa
                if (params.crossover) {
                    while (newPop.size() < params.populationSize) {
                        int p1_idx = parentIndices[parentDist(rng)];
                        int p2_idx = parentIndices[parentDist(rng)];

                        Genome childGenes = crossover(population[p1_idx].genes, population[p2_idx].genes);
                        mutate(childGenes);

                        Individual child;
                        child.genes = childGenes;
                        child.avgFitness = -DBL_MAX;
                        child.id = nextIndId++;
                        newPop.push_back(child);
                    }
                } else {
                    while (newPop.size() < params.populationSize) {
                        int p_idx = parentIndices[parentDist(rng)];

                        Genome childGenes = population[p_idx].genes;
                        mutate(childGenes);

                        Individual child;
                        child.genes = childGenes;
                        child.avgFitness = -DBL_MAX;
                        child.id = nextIndId++;
                        newPop.push_back(child);
                    }
                }

                population = std::move(newPop);

                // Zmiejszanie mutacji w kolejnych generacjach
                if (params.mutationAnnealing && params.mutationSigma > params.minSigma) {
                    params.mutationSigma *= params.decayFactor;
                    if (params.mutationSigma < params.minSigma) {
                        params.mutationSigma = params.minSigma;
                    }
                }
                if (std::binary_search(targetGens.begin(), targetGens.end(), gen)) {
                    for (const auto &ind: population) {
                        finalPopulations.push_back(ind);
                    }
                    std::cout << "[COLLECT] Gen " << gen << " | Collected population of size " << population.size() <<
                            " | Total collected: " << finalPopulations.size() << std::endl;
                }
            }
        }

        void run_normal() {
            int genomeSize = heuristicPrototype.getParamsSize();
            std::cout << "Starting Normal Evolution. Genome size (weights): " << genomeSize << std::endl;

            if (params.mutationAnnealing) {
                if (params.mutationSigma > params.minSigma && params.generations > 0) {
                    params.decayFactor = std::pow(params.minSigma / params.mutationSigma, 1.0 / params.generations);
                } else {
                    std::cout << "Stopping early: Mutation Annealing parameters conflict." << std::endl;
                    return;
                }
            }
            std::vector<int> targetGens = calculateTargetGenerations();
            for (int gen = 0; gen < params.generations; ++gen) {
                // Wybór losowego batcha zadań
                std::vector<BinpackData> batch;
                std::sample(allTrainingData.begin(), allTrainingData.end(),
                            std::back_inserter(batch), params.batchSize, rng);

                /// Ewaluacja calej populacji sieci dla wszystkich zadań (avg)
                evaluatePopulationNormal(batch);
                // Sortowanie populacji malejąco po fitness
                std::sort(population.begin(), population.end(),
                          [](const Individual &a, const Individual &b) {
                              return a.avgFitness > b.avgFitness;
                          });

                double bestFit = population[0].avgFitness;
                std::cout << "Gen " << gen << " | Best Fitness (Avg Fill Factor): " << bestFit << std::endl;

                // Tworzenie nowej populacji
                std::vector<Individual> newPop;
                newPop.reserve(params.populationSize);

                // Elityzm
                if (params.elitism)
                    for (int i = 0; i < params.eliteSize; ++i) {
                        newPop.push_back(population[i]);
                    }

                // Jeśli crossover to krzyzowanie i mutacja, jesli nie to tylko mutacja
                // Rodzice wybierani selekcja turniejowa
                if (params.crossover) {
                    while (newPop.size() < params.populationSize) {
                        const auto p1 = tournamentSelect();
                        const auto p2 = tournamentSelect();

                        Genome childGenes = crossover(p1.genes, p2.genes);
                        mutate(childGenes);

                        newPop.push_back({childGenes, -DBL_MAX});
                    }
                } else {
                    while (newPop.size() < params.populationSize) {
                        const auto p1 = tournamentSelect();

                        Genome childGenes = p1.genes;
                        mutate(childGenes);

                        newPop.push_back({childGenes, -DBL_MAX});
                    }
                }

                population = std::move(newPop);

                // Zmiejszanie mutacji w kolejnych generacjach
                if (params.mutationAnnealing && params.mutationSigma > params.minSigma) {
                    params.mutationSigma *= params.decayFactor;
                    if (params.mutationSigma < params.minSigma) {
                        params.mutationSigma = params.minSigma;
                    }
                }

                // Zbieranie populacji z ostatnich N generacji co K generacji
                if (std::binary_search(targetGens.begin(), targetGens.end(), gen)) {
                    for (const auto &ind: population) {
                        finalPopulations.push_back(ind);
                    }
                    std::cout << "[COLLECT] Gen " << gen << " | Collected population of size " << population.size() <<
                            " | Total collected: " << finalPopulations.size() << std::endl;
                }
            }
        }

        Genome crossover(const Genome &p1, const Genome &p2) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            Genome child = p1;
            if (dist(rng) < params.crossoverRate) {
                std::bernoulli_distribution pickParent(0.5);
                for (size_t i = 0; i < p1.size(); ++i) {
                    child[i] = pickParent(rng) ? p1[i] : p2[i];
                }
            }
            return child;
        }

        void mutate(Genome &genome) {
            std::uniform_real_distribution<double> prob(0.0, 1.0);
            std::normal_distribution<double> noise(0.0, params.mutationSigma);
            for (double &gene: genome) {
                if (prob(rng) < params.mutationRate) {
                    gene += noise(rng);
                }
            }
        }

        const Individual &tournamentSelect() {
            std::uniform_int_distribution<int> dist(0, params.populationSize - 1);
            int bestIdx = dist(rng);

            for (int i = 1; i < params.tournamentSize; ++i) {
                int contestIdx = dist(rng);
                if (population[contestIdx].avgFitness > population[bestIdx].avgFitness) {
                    bestIdx = contestIdx;
                }
            }
            return population[bestIdx];
        }

        void evaluatePopulationNormal(const std::vector<BinpackData> &batch) {
#pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < population.size(); ++i) {
                // Kopia heurystyki dla każdego wątku
                HeuristicType localHeuristic = heuristicPrototype;

                // Ustawienie wag z genotypu
                localHeuristic.setParams(population[i].genes.data(), population[i].genes.size());

                double totalFillFactor = 0.0;
                for (const auto &instance: batch) {
                    auto solution = localHeuristic.run(instance);
                    totalFillFactor += solution.getObj();
                }
                // średni fill factor dla każdej z sieci
                population[i].avgFitness = totalFillFactor / batch.size();
            }
        }

        // Ewaluacja populacji na dowolnym zbiorze danych
        std::vector<double> evaluatePopulation(const std::vector<BinpackData> &data) {
            std::vector<double> results(population.size(), 0.0);
#pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < population.size(); ++i) {
                // Kopia heurystyki dla każdego wątku
                HeuristicType localHeuristic = heuristicPrototype;
                // Ustawienie wag z genotypu
                localHeuristic.setParams(population[i].genes.data(), population[i].genes.size());
                double total = 0.0;
                for (const auto &instance: data) {
                    auto solution = localHeuristic.run(instance);
                    total += solution.getObj();
                }
                results[i] = total / data.size();
            }
            return results;
        }

        std::vector<Individual> getPopulation() const {
            return population;
        }

        std::vector<Individual> getFinalPopulations() const {
            return finalPopulations;
        }

        std::vector<int> calculateTargetGenerations() const {
            std::vector<int> targetGens;
            if (params.numPopulationsToCollect > 0) {
                int startGen = static_cast<int>(params.generations * params.collectionStartPercent);
                startGen = std::max(0, std::min(startGen, params.generations - 1));

                if (params.numPopulationsToCollect == 1) {
                    targetGens.push_back(params.generations - 1);
                } else {
                    double step = static_cast<double>(params.generations - 1 - startGen) / (
                                      params.numPopulationsToCollect - 1);
                    for (int i = 0; i < params.numPopulationsToCollect; ++i) {
                        int target = startGen + static_cast<int>(std::round(i * step));
                        targetGens.push_back(target);
                    }
                }
            }
            return targetGens;
        }

        // Ewaluuje zebrane populacje na zbiorze danych i zwraca wyniki
        std::vector<double> evaluateFinalPopulations(const std::vector<BinpackData> &data) {
            std::vector<double> results(finalPopulations.size(), 0.0);
#pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < finalPopulations.size(); ++i) {
                // Kopia heurystyki dla każdego wątku
                HeuristicType localHeuristic = heuristicPrototype;
                // Ustawienie wag z genotypu
                localHeuristic.setParams(finalPopulations[i].genes.data(), finalPopulations[i].genes.size());
                double total = 0.0;
                for (const auto &instance: data) {
                    auto solution = localHeuristic.run(instance);
                    total += solution.getObj();
                }
                results[i] = total / data.size();
            }
            return results;
        }

        // 1. Zbudowanie macierzy wyników TYLKO RAZ
        std::vector<std::vector<double> > buildScoreMatrix(const std::vector<Individual> &candidatePool,
                                                           const std::vector<BinpackData> &data) {
            int numIndividuals = candidatePool.size();
            int numInstances = data.size();
            std::vector<std::vector<double> > scoreMatrix(numIndividuals, std::vector<double>(numInstances, 0.0));

            if (numIndividuals == 0 || numInstances == 0) return scoreMatrix;

            std::cout << "Building score matrix for " << numIndividuals << " individuals on " << numInstances <<
                    " instances..." << std::endl;

#pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < numIndividuals; ++i) {
                HeuristicType localHeuristic = heuristicPrototype;
                localHeuristic.setParams(candidatePool[i].genes.data(), candidatePool[i].genes.size());

                for (int j = 0; j < numInstances; ++j) {
                    auto solution = localHeuristic.run(data[j]);
                    scoreMatrix[i][j] = solution.getObj(); // Zakładamy, że wyższy wynik (fill factor) jest lepszy
                }
            }
            return scoreMatrix;
        }

        // 2. Szybki wybór specjalistów na podstawie gotowej macierzy (bez ewaluacji sieci)
        std::vector<Individual> selectSpecialistsFromMatrix(const std::vector<Individual> &candidatePool,
                                                            const std::vector<std::vector<double> > &scoreMatrix,
                                                            int subsetSize) {
            int numIndividuals = candidatePool.size();
            if (numIndividuals == 0 || scoreMatrix.empty() || subsetSize <= 0) return {};
            int numInstances = scoreMatrix[0].size();

            std::vector<int> selectedIndices;
            std::vector<bool> isSelected(numIndividuals, false);
            std::vector<double> currentBestScores(numInstances, 0.0);

            for (int k = 0; k < subsetSize; ++k) {
                double bestMarginalGain = -1.0;
                int bestCandidate = -1;

                // Szukanie kandydata z największym przyrostem marginalnym
                for (int i = 0; i < numIndividuals; ++i) {
                    if (isSelected[i]) continue;

                    double marginalGain = 0.0;
                    for (int j = 0; j < numInstances; ++j) {
                        if (scoreMatrix[i][j] > currentBestScores[j]) {
                            marginalGain += (scoreMatrix[i][j] - currentBestScores[j]);
                        }
                    }

                    if (marginalGain > bestMarginalGain) {
                        bestMarginalGain = marginalGain;
                        bestCandidate = i;
                    }
                }

                if (bestCandidate != -1 && bestMarginalGain > 0.0) {
                    selectedIndices.push_back(bestCandidate);
                    isSelected[bestCandidate] = true;

                    // Aktualizacja tabeli z najlepszymi obecnymi wynikami dla instancji
                    for (int j = 0; j < numInstances; ++j) {
                        if (scoreMatrix[bestCandidate][j] > currentBestScores[j]) {
                            currentBestScores[j] = scoreMatrix[bestCandidate][j];
                        }
                    }
                    std::cout << "Specialist " << k + 1 << "/" << subsetSize << " chosen. Marginal Gain: " <<
                            bestMarginalGain << std::endl;
                } else {
                    std::cout << "Stopping early: No further improvement possible." << std::endl;
                    break;
                }
            }

            // Składanie docelowej grupy
            std::vector<Individual> specialistSet;
            for (int idx: selectedIndices) {
                specialistSet.push_back(candidatePool[idx]);
            }
            return specialistSet;
        }

        std::vector<double> getBestWeights() {
            return population[0].genes;
        }

        std::vector<double> getBestWeightsFromValidation() {
            if (finalPopulations.empty()) {
                std::cout << "No populations collected. Falling back to best training individual." << std::endl;
                return getBestWeights();
            }
            if (validationData.empty()) {
                std::cout << "Warning: Validation dataset is empty! Falling back to best training individual." <<
                        std::endl;
                return getBestWeights();
            }

            // Ewaluacja zebranych osobników na wbudowanym zbiorze walidacyjnym
            std::cout << "Evaluating " << finalPopulations.size() << " collected individuals on the validation set..."
                    << std::endl;
            std::vector<double> validationScores = evaluateFinalPopulations(validationData);

            // Wybór jednego, globalnie najlepszego osobnika
            double bestScore = -DBL_MAX;
            int bestIdx = -1;
            for (size_t i = 0; i < validationScores.size(); ++i) {
                if (validationScores[i] > bestScore) {
                    bestScore = validationScores[i];
                    bestIdx = i;
                }
            }

            std::cout << "Best global model found on validation set!" << std::endl;
            std::cout << "Validation Avg Fill Factor: " << bestScore
                    << " (from collected individual #" << bestIdx << ")" << std::endl;

            return finalPopulations[bestIdx].genes;
        }

        // Zwraca populacje bez duplikatow
        std::vector<std::vector<double> > getUniquePopulation() {
            std::vector<std::vector<double> > uniqueWeights;
            for (const auto &ind: population) {
                uniqueWeights.push_back(ind.genes);
            }
            return uniqueWeights;
        }

        void setFinalPopulations(const std::vector<Individual> &loadedPopulations) {
            finalPopulations = loadedPopulations;
        }
    };
}

