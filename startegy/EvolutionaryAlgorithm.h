#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include <memory>
#include <omp.h>
#include <set>
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
        bool elitism = true;
        int eliteSize = populationSize/10;
        int tournamentSize = 5;
        bool crossover = false;
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
        std::vector<BinpackData> allTrainingData;

        std::mt19937 rng;
        std::vector<Individual> population;
        int nextIndId = 0;

    public:
        EvolutionaryAlgorithm(const EvoParams& _params,
                              const HeuristicType& _heuristic,
                              const std::vector<BinpackData>& _data)
            : params(_params), heuristicPrototype(_heuristic), allTrainingData(_data) {

            std::random_device rd;
            rng.seed(rd());

            initPopulation();
        }

        void initPopulation() {
            int genomeSize = heuristicPrototype.getParamsSize();
            population.resize(params.populationSize);

            std::normal_distribution<double> dist(0.0, 0.1);

            for (auto& ind : population) {
                ind.genes.resize(genomeSize);
                for (double& gene : ind.genes) {
                    gene = dist(rng);
                }
                ind.avgFitness = -DBL_MAX;
                ind.id = nextIndId++;
            }
        }

        void run() {
            int genomeSize = heuristicPrototype.getParamsSize();
            std::cout << "Starting Specialist Evolution. Genome size: " << genomeSize << std::endl;

            for (int gen = 0; gen < params.generations; ++gen) {

                // Wybór losowego batcha zadań
                std::vector<BinpackData> batch;
                std::sample(allTrainingData.begin(), allTrainingData.end(),
                            std::back_inserter(batch), params.batchSize, rng);

                // Macierz wyników: [Siec][Zadanie] -> wynik (fill factor)
                std::vector<std::vector<double>> scores(params.populationSize, std::vector<double>(batch.size()));

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
                std::cout << "Gen " << gen << "..."<< std::endl;

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
                    for(int idx : uniqueWinners) {
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
                }else {
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
                if (params.mutationAnnealing && gen % 20 == 0 && params.mutationSigma > 0.05) {
                    params.mutationSigma *= 0.98;
                }
            }
        }

        void run_normal() {
            int genomeSize = heuristicPrototype.getParamsSize();
            std::cout << "Starting Normal Evolution. Genome size (weights): " << genomeSize << std::endl;

            for (int gen = 0; gen < params.generations; ++gen) {

                // Wybór losowego batcha zadań
                std::vector<BinpackData> batch;
                std::sample(allTrainingData.begin(), allTrainingData.end(),
                            std::back_inserter(batch), params.batchSize, rng);

                /// Ewaluacja calej populacji sieci dla wszystkich zadań (avg)
                evaluatePopulationNormal(batch);
                // Sortowanie populacji malejąco po fitness
                std::sort(population.begin(), population.end(),
                    [](const Individual& a, const Individual& b) {
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
                        const auto& p1 = tournamentSelect();
                        const auto& p2 = tournamentSelect();

                        Genome childGenes = crossover(p1.genes, p2.genes);
                        mutate(childGenes);

                        newPop.push_back({childGenes, -DBL_MAX});
                    }
                }else {
                    while (newPop.size() < params.populationSize) {
                        const auto& p1 = tournamentSelect();

                        Genome childGenes = p1.genes;
                        mutate(childGenes);

                        newPop.push_back({childGenes, -DBL_MAX});
                    }
                }

                population = std::move(newPop);

                // Zmiejszanie mutacji w kolejnych generacjach
                if (params.mutationAnnealing && gen % 20 == 0 && params.mutationSigma > 0.05) {
                    params.mutationSigma *= 0.98;
                }
            }
        }

        Genome crossover(const Genome& p1, const Genome& p2) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            Genome child = p1;
            if (dist(rng) < params.crossoverRate) {
                for (size_t i = 0; i < p1.size(); ++i) {
                    child[i] = (p1[i] + p2[i]) / 2.0;
                }
            }
            return child;
        }

        void mutate(Genome& genome) {
            std::uniform_real_distribution<double> prob(0.0, 1.0);
            std::normal_distribution<double> noise(0.0, params.mutationSigma);
            for (double& gene : genome) {
                if (prob(rng) < params.mutationRate) {
                    gene += noise(rng);
                }
            }
        }

        const Individual& tournamentSelect() {
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

        void evaluatePopulationNormal(const std::vector<BinpackData>& batch) {
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < population.size(); ++i) {
                // Kopia heurystyki dla każdego wątku
                HeuristicType localHeuristic = heuristicPrototype;

                // Ustawienie wag z genotypu
                localHeuristic.setParams(population[i].genes.data(), population[i].genes.size());

                double totalFillFactor = 0.0;
                for (const auto& instance : batch) {
                    auto solution = localHeuristic.run(instance);
                    totalFillFactor += solution.getObj();
                }
                // średni fill factor dla każdej z sieci
                population[i].avgFitness = totalFillFactor / batch.size();
            }
        }

        std::vector<Individual> getPopulation() const {
            return population;
        }

        std::vector<double> getBestWeights() {
            return population[0].genes;
        }

        // Zwraca populacje bez duplikatow
        std::vector<std::vector<double>> getUniquePopulation() {
            std::vector<std::vector<double>> uniqueWeights;
            for(const auto& ind : population) {
                uniqueWeights.push_back(ind.genes);
            }
            return uniqueWeights;
        }
    };
}