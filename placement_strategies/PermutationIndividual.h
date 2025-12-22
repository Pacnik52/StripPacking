#pragma once
#include "Individual.h"
#include "MaxRectsStrategy.h"
#include <vector>
#include <random>

namespace binpack {

    class PermutationIndividual : public Individual {
    public:
        // Genotype: a list of indices pointing to the original boxes vector
        std::vector<int> genotype;

        // We need a pointer to the strategy used for evaluation (decoder)
        // We use a shared_ptr to share one strategy instance across individuals for efficiency
        std::shared_ptr<MaxRectsStrategy> decoder;

        PermutationIndividual(size_t genomeSize, std::shared_ptr<MaxRectsStrategy> decoderStrategy);

        void randomize() override;

        // Runs MaxRects with the specific order defined in 'genotype'
        double evaluate(const std::vector<BinpackData::BoxType>& originalBoxes, BinpackData& data) override;

        std::unique_ptr<Individual> crossover(const Individual& other) const override;
        void mutate(double mutationRate) override;
        std::unique_ptr<Individual> clone() const override;

    private:
        // Helper for Order Crossover (OX1)
        std::vector<int> performOX1(const std::vector<int>& p1, const std::vector<int>& p2) const;
    };

}