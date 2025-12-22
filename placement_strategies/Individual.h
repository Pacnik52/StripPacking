#pragma once
#include <vector>
#include <memory>
#include "../BinpackData.h"

namespace binpack {

    class Individual {
    public:
        double fitness = -1.0;

        virtual ~Individual() = default;

        // Creates a random individual
        virtual void randomize() = 0;

        // Calculates fitness using the provided logic (MaxRects)
        // Returns the height (lower is better)
        virtual double evaluate(const std::vector<BinpackData::BoxType>& originalBoxes,
                                BinpackData& data) = 0;

        // Genetic Operators
        virtual std::unique_ptr<Individual> crossover(const Individual& other) const = 0;
        virtual void mutate(double mutationRate) = 0;

        // Helper to clone self
        virtual std::unique_ptr<Individual> clone() const = 0;
    };

}