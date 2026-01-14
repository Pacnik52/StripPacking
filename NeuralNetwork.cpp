// NeuralNetwork.cpp
#include "NeuralNetwork.h"
#include <cmath>

NeuralNetwork::NeuralNetwork() {
    W1.resize(32, 22); b1.resize(32);
    W2.resize(12, 32); b2.resize(12);
    W3.resize(1, 12);  b3.resize(1);
}

Eigen::VectorXd NeuralNetwork::activationTanh(const Eigen::VectorXd& x) const {
    return x.array().tanh();
}

void NeuralNetwork::setWeights(const double* genome) {
    int idx = 0;
    // Warstwa 1
    for(int i=0; i<32; ++i) {
        b1(i) = genome[idx++];
        for(int j=0; j<22; ++j) W1(i,j) = genome[idx++];
    }
    // Warstwa 2
    for(int i=0; i<12; ++i) {
        b2(i) = genome[idx++];
        for(int j=0; j<32; ++j) W2(i,j) = genome[idx++];
    }
    // Wyjście
    for(int i=0; i<1; ++i) {
        b3(i) = genome[idx++];
        for(int j=0; j<12; ++j) W3(i,j) = genome[idx++];
    }
}

double NeuralNetwork::predict(const std::vector<double>& inputs) {
    Eigen::Map<const Eigen::VectorXd> x(inputs.data(), inputs.size());

    Eigen::VectorXd h1 = activationTanh(W1 * x + b1);
    Eigen::VectorXd h2 = activationTanh(W2 * h1 + b2);
    // Wyjście liniowe (zgodnie z artykułem)
    Eigen::VectorXd out = W3 * h2 + b3;

    return out(0);
}

int NeuralNetwork::getParameterCount() const {
    return (32*22 + 32) + (12*32 + 12) + (1*12 + 1); // 1145
}