// NeuralStrategy.h
#pragma once
#include "BinPackData.h"
#include "NeuralNetwork.h"

class NeuralStrategy {
private:
    NeuralNetwork& net;

public:
    NeuralStrategy(NeuralNetwork& network);

    // Rozwiązuje pojedynczą instancję problemu
    // Zwraca Fill Factor (0.0 - 1.0) oraz modyfikuje solution w data
    double solve(binpack::BinpackData& data);
};