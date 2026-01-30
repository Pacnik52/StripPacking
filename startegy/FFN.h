#pragma once
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <Eigen/Dense>
#include <boost/serialization/vector.hpp>
#include <filesystem>

namespace nnutils {
    using namespace std;
    using namespace Eigen;

    class FFN {
        friend class boost::serialization::access;

    public:
        struct Config {
            friend class boost::serialization::access;

            int inputSize = 25;
            int hidden1Size = 32;
            int hidden2Size = 12;
            int outputSize = 1;

            template<class Archive>
            void serialize(Archive &ar, const unsigned int version) {
                ar & inputSize;
                ar & hidden1Size;
                ar & hidden2Size;
                ar & outputSize;
            }
        };

    private:
        Config conf;

        // Macierze wag i wektory biasów
        MatrixXd W1, W2, W3;
        VectorXd b1, b2, b3;

        // Całkowita liczba parametrów
        int totalParams;

    public:
        FFN() : totalParams(0) {
        }

        explicit FFN(const Config &_conf) : conf(_conf) {
            init();
        }

        void init() {
            // Obliczenie rozmiaru wektora parametrów
            // Wagi + Biasy dla każdej warstwy
            totalParams = (conf.inputSize * conf.hidden1Size) + conf.hidden1Size +
                          (conf.hidden1Size * conf.hidden2Size) + conf.hidden2Size +
                          (conf.hidden2Size * conf.outputSize) + conf.outputSize;

            // Inicjalizacja macierzy zerami (zostaną nadpisane przez setParams)
            W1 = MatrixXd::Zero(conf.hidden1Size, conf.inputSize);
            b1 = VectorXd::Zero(conf.hidden1Size);
            W2 = MatrixXd::Zero(conf.hidden2Size, conf.hidden1Size);
            b2 = VectorXd::Zero(conf.hidden2Size);
            W3 = MatrixXd::Zero(conf.outputSize, conf.hidden2Size);
            b3 = VectorXd::Zero(conf.outputSize);
        }

        int getParamsSize() const {
            return totalParams;
        }

        // Zapisuje sieć do pliku tekstowego
        void save(const std::string &outputDir, const std::string &filename) {
            namespace fs = std::filesystem;
            fs::create_directories(outputDir);
            fs::path filePath = fs::path(outputDir) / filename;
            std::ofstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for writing: " << filePath << std::endl;
                return;
            }

            file << conf.inputSize << " " << conf.hidden1Size << " "
                    << conf.hidden2Size << " " << conf.outputSize << "\n";

            auto writeMatrix = [&](const MatrixXd &m) {
                for (int r = 0; r < m.rows(); ++r) {
                    for (int c = 0; c < m.cols(); ++c) {
                        file << m(r, c) << " ";
                    }
                    file << "\n";
                }
            };

            auto writeVector = [&](const VectorXd &v) {
                for (int i = 0; i < v.size(); ++i) {
                    file << v(i) << " ";
                }
                file << "\n";
            };

            writeMatrix(W1);
            writeVector(b1);
            writeMatrix(W2);
            writeVector(b2);
            writeMatrix(W3);
            writeVector(b3);

            file.close();
            std::cout << "Network saved to " << filePath << std::endl;
        }

        // Wczytuje sieć z pliku
        bool load(const std::string &filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for reading: " << filename << std::endl;
                return false;
            }

            file >> conf.inputSize >> conf.hidden1Size >> conf.hidden2Size >> conf.outputSize;

            init();

            std::vector<double> weights;
            weights.reserve(totalParams);
            double val;

            while (weights.size() < totalParams && file >> val) {
                weights.push_back(val);
            }

            if (weights.size() != totalParams) {
                std::cerr << "Error: File format mismatch. Expected " << totalParams
                        << " weights, got " << weights.size() << std::endl;
                return false;
            }
            setParams(weights.data(), weights.size());

            file.close();
            std::cout << "Network loaded from " << filename << std::endl;
            return true;
        }

        // Mapowanie płaskiego wektora parametrów na macierze Eigen
        void setParams(const double *params, int n) {
            if (n != totalParams) {
                std::cerr << "Error: Parameter size mismatch. Expected " << totalParams << ", got " << n << std::endl;
                return;
            }

            int offset = 0;

            // Warstwa 1
            int w1_size = conf.inputSize * conf.hidden1Size;
            W1 = Map<const Matrix<double, Dynamic, Dynamic, RowMajor>>(params + offset, conf.hidden1Size,
                                                                       conf.inputSize);
            offset += w1_size;

            b1 = Map<const VectorXd>(params + offset, conf.hidden1Size);
            offset += conf.hidden1Size;

            // Warstwa 2
            int w2_size = conf.hidden1Size * conf.hidden2Size;
            W2 = Map<const Matrix<double, Dynamic, Dynamic, RowMajor>>(params + offset, conf.hidden2Size,
                                                                       conf.hidden1Size);
            offset += w2_size;

            b2 = Map<const VectorXd>(params + offset, conf.hidden2Size);
            offset += conf.hidden2Size;

            // Warstwa Wyjściowa
            int w3_size = conf.hidden2Size * conf.outputSize;
            W3 = Map<const Matrix<double, Dynamic, Dynamic, RowMajor>>(params + offset, conf.outputSize,
                                                                       conf.hidden2Size);
            offset += w3_size;

            b3 = Map<const VectorXd>(params + offset, conf.outputSize);
        }

        void getParams(double *params, int n) const {
            if (n != totalParams) {
                std::cerr << "Error: Parameter size mismatch. Expected " << totalParams << ", got " << n << std::endl;
                return;
            }
            int offset = 0;
            // Warstwa 1
            for (int r = 0; r < W1.rows(); ++r)
                for (int c = 0; c < W1.cols(); ++c)
                    params[offset++] = W1(r, c);
            for (int i = 0; i < b1.size(); ++i)
                params[offset++] = b1(i);
            // Warstwa 2
            for (int r = 0; r < W2.rows(); ++r)
                for (int c = 0; c < W2.cols(); ++c)
                    params[offset++] = W2(r, c);
            for (int i = 0; i < b2.size(); ++i)
                params[offset++] = b2(i);
            // Warstwa 3
            for (int r = 0; r < W3.rows(); ++r)
                for (int c = 0; c < W3.cols(); ++c)
                    params[offset++] = W3(r, c);
            for (int i = 0; i < b3.size(); ++i)
                params[offset++] = b3(i);
        }


        double operator()(const float *inputs, int size) {
            if (size != conf.inputSize) {
                return -1e9;
            }

            // Konwersja float array na VectorXd
            VectorXd x = Map<const VectorXf>(inputs, size).cast<double>();

            // Warstwa 1: Linear + Tanh
            VectorXd h1 = (W1 * x + b1).array().tanh();

            // Warstwa 2: Linear + Tanh
            VectorXd h2 = (W2 * h1 + b2).array().tanh();

            // Warstwa Wyjściowa
            VectorXd out = W3 * h2 + b3;

            return out(0);
        }

        // Zapisuje całą populację sieci do plików w podanym folderze
        static void save_population(const std::string &outputDir,
                                    const std::vector<std::vector<double> > &populationGenomes, const Config &conf) {
            namespace fs = std::filesystem;
            // Usuwanie wszystkich plików z outputDir
            if (std::filesystem::exists(outputDir)) {
                for (const auto &entry: std::filesystem::directory_iterator(outputDir)) {
                    if (entry.is_regular_file()) {
                        std::string ext = entry.path().extension().string();
                        std::filesystem::remove(entry.path());
                    }
                }
            } else {
                std::filesystem::create_directory(outputDir);
            }
            for (size_t i = 0; i < populationGenomes.size(); ++i) {
                FFN net(conf);
                net.setParams(populationGenomes[i].data(), populationGenomes[i].size());
                std::string filename = "specialist_" + std::to_string(i);
                net.save(outputDir, filename);
            }
        }

        static std::vector<std::vector<double> > load_population(const std::string &inputDir) {
            namespace fs = std::filesystem;
            std::vector<std::vector<double> > population;

            if (!fs::exists(inputDir)) {
                std::cerr << "Error: Directory not found: " << inputDir << std::endl;
                return population;
            }

            std::vector<std::pair<int, std::vector<double> > > indexedWeights;
            FFN tempNet;

            for (const auto &entry: fs::directory_iterator(inputDir)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    std::string filepath = entry.path().string();

                    // Iterowanie po wszytskich plikach specialist_
                    if (filename.find("specialist_") == 0) {
                        if (tempNet.load(filepath)) {
                            int pSize = tempNet.getParamsSize();
                            std::vector<double> weights(pSize);
                            tempNet.getParams(weights.data(), pSize);

                            int id = -1;
                            try {
                                size_t underscorePos = filename.find_last_of('_');
                                size_t dotPos = filename.find_last_of('.');
                                std::string numStr;
                                if (dotPos != std::string::npos && dotPos > underscorePos)
                                    numStr = filename.substr(underscorePos + 1, dotPos - underscorePos - 1);
                                else
                                    numStr = filename.substr(underscorePos + 1);

                                id = std::stoi(numStr);
                            } catch (...) {
                                id = 999999;
                            }

                            indexedWeights.push_back({id, weights});
                        }
                    }
                }
            }

            // Finalna populacja sieci
            std::sort(indexedWeights.begin(), indexedWeights.end(),
                      [](const auto &a, const auto &b) { return a.first < b.first; });
            population.reserve(indexedWeights.size());
            for (const auto &item: indexedWeights) {
                population.push_back(item.second);
            }

            std::cout << "Loaded " << population.size() << " specialist networks from " << inputDir << std::endl;
            return population;
        }
    };
}
