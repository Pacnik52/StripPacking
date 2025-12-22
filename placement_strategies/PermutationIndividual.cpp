#include "PermutationIndividual.h"
#include <algorithm>
#include <set>
#include <iostream>

namespace binpack {

using namespace std;

// Static RNG for performance
static std::mt19937 rng(std::random_device{}());

PermutationIndividual::PermutationIndividual(size_t genomeSize, std::shared_ptr<MaxRectsStrategy> decoderStrategy)
    : decoder(std::move(decoderStrategy)) {
    genotype.resize(genomeSize);
    // Fill with 0, 1, 2, ..., N-1
    std::iota(genotype.begin(), genotype.end(), 0);
}

void PermutationIndividual::randomize() {
    std::shuffle(genotype.begin(), genotype.end(), rng);
}

double PermutationIndividual::evaluate(const std::vector<BinpackData::BoxType>& originalBoxes, BinpackData& data) {
    // 1. Reorder boxes based on genotype
    std::vector<BinpackData::BoxType> orderedBoxes;
    orderedBoxes.reserve(originalBoxes.size());
    
    for (int idx : genotype) {
        orderedBoxes.push_back(originalBoxes[idx]);
    }

    // 2. Run MaxRects on this specific order
    // IMPORTANT: decoder->place usually writes to data.Solution. 
    // We pass 'data' directly.
    decoder->place(orderedBoxes, data);

    // 3. Get result (Objective is usually Max Height used)
    this->fitness = data.Solution.getObj();
    return this->fitness;
}

std::unique_ptr<Individual> PermutationIndividual::clone() const {
    auto copy = std::make_unique<PermutationIndividual>(genotype.size(), decoder);
    copy->genotype = this->genotype;
    copy->fitness = this->fitness;
    return copy;
}

void PermutationIndividual::mutate(double mutationRate) {
    // Swap Mutation
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    if (dist(rng) < mutationRate) {
        std::uniform_int_distribution<size_t> idxDist(0, genotype.size() - 1);
        size_t i = idxDist(rng);
        size_t j = idxDist(rng);
        std::swap(genotype[i], genotype[j]);
    }
}

std::unique_ptr<Individual> PermutationIndividual::crossover(const Individual& other) const {
    // Cast strictly to PermutationIndividual
    const auto* p2 = dynamic_cast<const PermutationIndividual*>(&other);
    if (!p2) return clone(); // Fallback

    auto child = std::make_unique<PermutationIndividual>(genotype.size(), decoder);
    
    // Perform Order Crossover (OX1)
    child->genotype = performOX1(this->genotype, p2->genotype);
    
    return child;
}

// Order Crossover (OX1) logic
std::vector<int> PermutationIndividual::performOX1(const std::vector<int>& p1, const std::vector<int>& p2) const {
    size_t size = p1.size();
    std::vector<int> child(size, -1);
    
    std::uniform_int_distribution<size_t> dist(0, size - 1);
    size_t start = dist(rng);
    size_t end = dist(rng);
    if (start > end) std::swap(start, end);

    std::set<int> inChild;

    // Copy segment from P1
    for (size_t i = start; i <= end; ++i) {
        child[i] = p1[i];
        inChild.insert(p1[i]);
    }

    // Fill remaining spots with order from P2
    size_t currentP2 = 0;
    for (size_t i = 0; i < size; ++i) {
        if (i >= start && i <= end) continue; // Skip existing segment

        while (inChild.count(p2[currentP2])) {
            currentP2++;
        }
        child[i] = p2[currentP2];
        inChild.insert(p2[currentP2]);
    }
    return child;
}

}