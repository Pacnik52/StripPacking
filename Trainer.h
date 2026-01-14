#pragma once
#include <vector>
#include <string>
#include "BinPackData.h"
#include "NeuralNetwork.h"

class Trainer {
public:
    static std::vector<binpack::BinpackData>* trainingData;

    void train(std::vector<binpack::BinpackData>& data, int generations);
    void saveBest(const std::string& filename);

    NeuralNetwork getBestNetwork() { return bestNet; }

private:
    NeuralNetwork bestNet;
};