#pragma once
#include "BinPackData.h"
#include "NeuralNetwork.h"

class NeuralStrategy {
private:
    NeuralNetwork& net;

public:
    NeuralStrategy(NeuralNetwork& network);

    double solve(binpack::BinpackData& data);
};