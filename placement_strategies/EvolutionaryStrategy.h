#pragma once
#include "PlacementStrategy.h"
#include "PermutationIndividual.h"
#include <vector>
#include <memory>

namespace binpack {

    // FIX: Defined OUTSIDE the class to allow default arguments in constructor
    struct EvolutionaryParams {
        int populationSize = 50;
        int generations = 100;
        double mutationRate = 0.2;
        int tournamentSize = 3;
    };

    class EvolutionaryStrategy : public PlacementStrategy {
    public:
        // Alias to maintain compatibility with main.cpp
        using Params = EvolutionaryParams;

        // Now Params{} is valid here because the struct is fully defined above
        explicit EvolutionaryStrategy(Params params = Params{});

        void place(const std::vector<BinpackData::BoxType>& boxes, BinpackData& data) override;

    private:
        Params params;
        std::vector<std::unique_ptr<Individual>> population;
        std::shared_ptr<MaxRectsStrategy> maxRectsDecoder;

        void initializePopulation(size_t genomeSize);
        std::unique_ptr<Individual> tournamentSelection();
    };

}