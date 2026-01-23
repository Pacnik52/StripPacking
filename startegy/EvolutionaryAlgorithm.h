#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>
#include <memory>
#include <omp.h> // Do zrównoleglenia oceny populacji
#include "BinpackConstructionHeuristic.h"
#include "../bin_reader/DataLoaderOdp.h"

namespace binpack {

    struct EvoParams {
        int populationSize = 100;     // Rozmiar populacji (artykuł używa 384 [cite: 366])
        int generations = 1000;       
        int batchSize = 100;          // Liczba instancji do oceny w jednej iteracji [cite: 284]
        double mutationRate = 0.1;
        double mutationSigma = 0.1;   // Odchylenie standardowe mutacji (dla wag)
        double crossoverRate = 0.8;
        int tournamentSize = 5;
        int eliteSize = 1;
    };

    class EvolutionaryAlgorithm {
    public:
        using HeuristicType = BinpackConstructionHeuristic<nnutils::FFN>;
        using Genome = std::vector<double>;

        struct Individual {
            Genome genes;
            double fitness;
        };

    private:
        EvoParams params;
        HeuristicType heuristicPrototype;
        std::vector<BinpackData> allTrainingData;
        
        std::mt19937 rng;
        std::vector<Individual> population;

    public:
        EvolutionaryAlgorithm(const EvoParams& _params, 
                              const HeuristicType& _heuristic, 
                              const std::vector<BinpackData>& _data)
            : params(_params), heuristicPrototype(_heuristic), allTrainingData(_data) {
            
            std::random_device rd;
            rng.seed(rd());
            
            initPopulation();
        }

        // Inicjalizacja wag losowymi wartościami z małego zakresu
        void initPopulation() {
            int genomeSize = heuristicPrototype.getParamsSize();
            population.resize(params.populationSize);

            std::normal_distribution<double> dist(0.0, 0.1); // Inicjalizacja bliska 0 [cite: 207]

            for (auto& ind : population) {
                ind.genes.resize(genomeSize);
                for (double& gene : ind.genes) {
                    gene = dist(rng);
                }
                ind.fitness = -DBL_MAX;
            }
        }

        // Ocena populacji na podzbiorze danych (Batch Evaluation )
        void evaluatePopulation(const std::vector<BinpackData>& batch) {
            #pragma omp parallel for schedule(dynamic)
            for (int i = 0; i < population.size(); ++i) {
                // Kopia heurystyki dla każdego wątku
                HeuristicType localHeuristic = heuristicPrototype;
                
                // Ustawienie wag z genotypu
                localHeuristic.setParams(population[i].genes.data(), population[i].genes.size());

                double totalFillFactor = 0.0;
                
                for (const auto& instance : batch) {
                    auto solution = localHeuristic.run(instance);
                    
                    // Funkcja celu: Fill Factor
                    // W BinpackConstructionHeuristic solution.getObj() zwraca już odpowiednią wartość 
                    // (dla StripPacking jest to density/fill factor)
                    totalFillFactor += solution.getObj();
                }
                
                // Średni Fill Factor [cite: 280]
                population[i].fitness = totalFillFactor / batch.size();
            }
        }

        // Selekcja turniejowa
        const Individual& tournamentSelect() {
            std::uniform_int_distribution<int> dist(0, params.populationSize - 1);
            int bestIdx = dist(rng);
            
            for (int i = 1; i < params.tournamentSize; ++i) {
                int contestIdx = dist(rng);
                if (population[contestIdx].fitness > population[bestIdx].fitness) {
                    bestIdx = contestIdx;
                }
            }
            return population[bestIdx];
        }

        // Krzyżowanie arytmetyczne
        Genome crossover(const Genome& p1, const Genome& p2) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            Genome child = p1;
            
            if (dist(rng) < params.crossoverRate) {
                for (size_t i = 0; i < p1.size(); ++i) {
                    // Uśrednienie wag (typowe dla zmiennoprzecinkowych GA)
                    child[i] = (p1[i] + p2[i]) / 2.0; 
                }
            }
            return child;
        }

        // Mutacja Gaussowska
        void mutate(Genome& genome) {
            std::uniform_real_distribution<double> prob(0.0, 1.0);
            std::normal_distribution<double> noise(0.0, params.mutationSigma);

            for (double& gene : genome) {
                if (prob(rng) < params.mutationRate) {
                    gene += noise(rng);
                }
            }
        }

        void run() {
            int genomeSize = heuristicPrototype.getParamsSize();
            std::cout << "Starting Evolution. Genome size (weights): " << genomeSize << std::endl;

            for (int gen = 0; gen < params.generations; ++gen) {
                
                // 1. Wybór batcha treningowego [cite: 283, 284]
                std::vector<BinpackData> batch;
                std::sample(allTrainingData.begin(), allTrainingData.end(), 
                            std::back_inserter(batch), params.batchSize, rng);

                // 2. Ewaluacja
                evaluatePopulation(batch);

                // Sortowanie populacji malejąco po fitness
                std::sort(population.begin(), population.end(), 
                    [](const Individual& a, const Individual& b) {
                        return a.fitness > b.fitness;
                    });

                double bestFit = population[0].fitness;
                std::cout << "Gen " << gen << " | Best Fitness (Avg Fill Factor): " << bestFit << std::endl;

                // 3. Tworzenie nowej populacji
                std::vector<Individual> newPop;
                newPop.reserve(params.populationSize);

                // Elityzm
                for (int i = 0; i < params.eliteSize; ++i) {
                    newPop.push_back(population[i]);
                }

                // Generowanie potomstwa
                while (newPop.size() < params.populationSize) {
                    const auto& p1 = tournamentSelect();
                    const auto& p2 = tournamentSelect();

                    Genome childGenes = crossover(p1.genes, p2.genes);
                    mutate(childGenes);

                    newPop.push_back({childGenes, -DBL_MAX});
                }

                population = std::move(newPop);
                
                // Zmniejszanie sigma mutacji w czasie (opcjonalnie, dla zbieżności)
                if (gen % 100 == 0 && params.mutationSigma > 0.01) {
                    params.mutationSigma *= 0.95;
                }
            }
        }

        // Zwraca wagi najlepszego osobnika
        std::vector<double> getBestWeights() {
            // Zakładamy, że populacja jest posortowana po ostatniej iteracji
            return population[0].genes;
        }
    };
}