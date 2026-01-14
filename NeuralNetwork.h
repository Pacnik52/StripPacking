// NeuralNetwork.h
#pragma once
#include <vector>
#include <Eigen/Dense>

class NeuralNetwork {
private:
    Eigen::MatrixXd W1, W2, W3;
    Eigen::VectorXd b1, b2, b3;

    Eigen::VectorXd activationTanh(const Eigen::VectorXd& x) const;

public:
    NeuralNetwork();

    // Ustawienie wag z płaskiego wektora (dla CMA-ES)
    void setWeights(const double* genome);

    // Ocena ruchu (Forward pass)
    double predict(const std::vector<double>& inputs);

    // Liczba parametrów (do konfiguracji CMA-ES)
    int getParameterCount() const;
};