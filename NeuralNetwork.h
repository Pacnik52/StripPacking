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

    void setWeights(const double* genome);

    double predict(const std::vector<double>& inputs);

    int getParameterCount() const;
};