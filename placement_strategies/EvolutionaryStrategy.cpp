#include "EvolutionaryStrategy.h"
#include "MaxRectsStrategy.h" // Ensure this is included for the decoder
#include <algorithm>
#include <iostream>
#include <random>

namespace binpack {

static std::mt19937 rngGen(std::random_device{}());

EvolutionaryStrategy::EvolutionaryStrategy(Params p) : params(p) {
    // We initialize the decoder (MaxRects) once
    maxRectsDecoder = std::make_shared<MaxRectsStrategy>();
}

void EvolutionaryStrategy::initializePopulation(size_t genomeSize) {
    population.clear();
    for (int i = 0; i < params.populationSize; ++i) {
        auto ind = std::make_unique<PermutationIndividual>(genomeSize, maxRectsDecoder);
        ind->randomize();
        population.push_back(std::move(ind));
    }
}

void EvolutionaryStrategy::place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) {
    // std::cout << "EA: Initializing population with " << params.populationSize << " individuals..." << std::endl;
    initializePopulation(boxes.size());

    // Best solution found so far (Genotype + Fitness)
    std::unique_ptr<Individual> bestGlobal = nullptr;

    for (int gen = 0; gen < params.generations; ++gen) {

        // Use a temporary data object to avoid constantly overwriting the main solution
        BinpackData tempResult = data;

        double bestGenFitness = 1e9;

        for (auto& ind : population) {
            // Evaluate modifies 'tempResult' and returns height
            double fit = ind->evaluate(boxes, tempResult);

            // Update Global Best
            if (!bestGlobal || fit < bestGlobal->fitness) {
                bestGlobal = ind->clone();
                // IMPORTANT: Save the actual layout to the real 'data' object
                data = tempResult;
                // std::cout << "Gen " << gen << " [New Best] Height: " << fit << std::endl;
            }

            if (fit < bestGenFitness) bestGenFitness = fit;
        }

        // Selection & Crossover (Create new population)
        std::vector<std::unique_ptr<Individual>> newPop;

        // Elitism: Always keep the champion
        if (bestGlobal) {
            newPop.push_back(bestGlobal->clone());
        }

        while (newPop.size() < params.populationSize) {
            auto p1 = tournamentSelection();
            auto p2 = tournamentSelection();

            auto child = p1->crossover(*p2);
            child->mutate(params.mutationRate);
            newPop.push_back(std::move(child));
        }

        population = std::move(newPop);
    }

    // Final step: Ensure 'data' contains the best solution found at the very end
    if (bestGlobal) {
        // std::cout << "EA Finished. Final Best Height: " << bestGlobal->fitness << std::endl;
        bestGlobal->evaluate(boxes, data);
    }
}

std::unique_ptr<Individual> EvolutionaryStrategy::tournamentSelection() {
    std::uniform_int_distribution<int> dist(0, population.size() - 1);

    int bestIdx = dist(rngGen);
    for (int i = 1; i < params.tournamentSize; ++i) {
        int contenderIdx = dist(rngGen);
        // Lower fitness (Height) is better
        if (population[contenderIdx]->fitness < population[bestIdx]->fitness) {
            bestIdx = contenderIdx;
        }
    }
    return population[bestIdx]->clone();
}

}