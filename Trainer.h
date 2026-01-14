#pragma once
#include <vector>
#include <string>
#include "BinPackData.h"
#include "NeuralNetwork.h"

// USUNIĘTO: struct cmaes_t; <- To powodowało błąd.
// Klasa Trainer nie potrzebuje wiedzieć co to jest cmaes_t w nagłówku.

class Trainer {
public:
    // Statyczny wskaźnik do danych (dla funkcji C)
    static std::vector<binpack::BinpackData>* trainingData;

    void train(std::vector<binpack::BinpackData>& data, int generations);
    void saveBest(const std::string& filename);

    NeuralNetwork getBestNetwork() { return bestNet; }

private:
    NeuralNetwork bestNet;
};