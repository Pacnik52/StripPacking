#pragma once
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <Eigen/Dense>
#include <boost/serialization/vector.hpp>

namespace nnutils {
    using namespace std;
    using namespace Eigen;

    class FFN {
        friend class boost::serialization::access;

    public:
        struct Config {
            friend class boost::serialization::access;
            
            // Domyślne wartości zgodne z artykułem [cite: 206]
            // Wejście: 25 (zgodnie z BinpackConstructionHeuristic.h), Ukryte: 32, 12, Wyjście: 1
            int inputSize = 25; 
            int hidden1Size = 32;
            int hidden2Size = 12;
            int outputSize = 1;

            template<class Archive>
            void serialize(Archive & ar, const unsigned int version) {
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

        // Całkowita liczba parametrów (do optymalizacji przez algorytm ewolucyjny)
        int totalParams;

    public:
        // Konstruktor domyślny wymagany przez serializację
        FFN() : totalParams(0) {}

        explicit FFN(const Config &_conf) : conf(_conf) {
            init();
        }

        void init() {
            // Obliczenie rozmiaru wektora parametrów (genotypu)
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
        void save(const std::string& filename) {
            std::ofstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for writing: " << filename << std::endl;
                return;
            }

            // 1. Zapisz konfigurację
            file << conf.inputSize << " " << conf.hidden1Size << " "
                 << conf.hidden2Size << " " << conf.outputSize << "\n";

            // 2. Helper lambda do zapisu macierzy (wierszami, bo setParams używa RowMajor)
            auto writeMatrix = [&](const MatrixXd& m) {
                for(int r=0; r<m.rows(); ++r) {
                    for(int c=0; c<m.cols(); ++c) {
                        file << m(r, c) << " ";
                    }
                    file << "\n";
                }
            };

            // Helper lambda do zapisu wektora
            auto writeVector = [&](const VectorXd& v) {
                for(int i=0; i<v.size(); ++i) {
                    file << v(i) << " ";
                }
                file << "\n";
            };

            // 3. Zapisz wagi w tej samej kolejności co w setParams
            writeMatrix(W1);
            writeVector(b1);
            writeMatrix(W2);
            writeVector(b2);
            writeMatrix(W3);
            writeVector(b3);

            file.close();
            std::cout << "Network saved to " << filename << std::endl;
        }

        // Wczytuje sieć z pliku
        bool load(const std::string& filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for reading: " << filename << std::endl;
                return false;
            }

            // 1. Wczytaj konfigurację
            file >> conf.inputSize >> conf.hidden1Size >> conf.hidden2Size >> conf.outputSize;

            // 2. Zreinicjalizuj macierze (ważne, jeśli wczytujemy inną architekturę)
            init();

            // 3. Wczytaj wszystkie wagi do jednego wektora
            std::vector<double> weights;
            weights.reserve(totalParams);
            double val;

            // Czytamy aż do końca pliku lub uzbierania odpowiedniej liczby parametrów
            while(weights.size() < totalParams && file >> val) {
                weights.push_back(val);
            }

            if (weights.size() != totalParams) {
                std::cerr << "Error: File format mismatch. Expected " << totalParams
                          << " weights, got " << weights.size() << std::endl;
                return false;
            }

            // 4. Ustaw wagi używając istniejącej logiki mapowania
            setParams(weights.data(), weights.size());

            file.close();
            std::cout << "Network loaded from " << filename << std::endl;
            return true;
        }

        // Mapowanie płaskiego wektora parametrów (z algorytmu ewolucyjnego) na macierze Eigen
        void setParams(const double *params, int n) {
            if (n != totalParams) {
                std::cerr << "Error: Parameter size mismatch. Expected " << totalParams << ", got " << n << std::endl;
                return;
            }

            int offset = 0;

            // Warstwa 1
            int w1_size = conf.inputSize * conf.hidden1Size;
            W1 = Map<const Matrix<double, Dynamic, Dynamic, RowMajor>>(params + offset, conf.hidden1Size, conf.inputSize);
            offset += w1_size;
            
            b1 = Map<const VectorXd>(params + offset, conf.hidden1Size);
            offset += conf.hidden1Size;

            // Warstwa 2
            int w2_size = conf.hidden1Size * conf.hidden2Size;
            W2 = Map<const Matrix<double, Dynamic, Dynamic, RowMajor>>(params + offset, conf.hidden2Size, conf.hidden1Size);
            offset += w2_size;

            b2 = Map<const VectorXd>(params + offset, conf.hidden2Size);
            offset += conf.hidden2Size;

            // Warstwa Wyjściowa
            int w3_size = conf.hidden2Size * conf.outputSize;
            W3 = Map<const Matrix<double, Dynamic, Dynamic, RowMajor>>(params + offset, conf.outputSize, conf.hidden2Size);
            offset += w3_size;

            b3 = Map<const VectorXd>(params + offset, conf.outputSize);
        }

        // Forward Pass - obliczenie oceny decyzji
        // Wejście jako float* dla zgodności z BinpackConstructionHeuristic
        double operator()(const float *inputs, int size) {
            if (size != conf.inputSize) {
                // Zabezpieczenie na wypadek niezgodności wymiarów
                return -1e9; 
            }

            // Konwersja float array na VectorXd
            VectorXd x = Map<const VectorXf>(inputs, size).cast<double>();

            // Warstwa 1: Linear + Tanh
            VectorXd h1 = (W1 * x + b1).array().tanh();

            // Warstwa 2: Linear + Tanh
            VectorXd h2 = (W2 * h1 + b2).array().tanh();

            // Warstwa Wyjściowa: Linear (skalarna ocena, brak aktywacji na wyjściu zgodnie z logiką oceny)
            // Artykuł mówi o biasach również na wyjściu [cite: 206]
            VectorXd out = W3 * h2 + b3;

            return out(0);
        }

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & conf;
            // Wagi nie muszą być serializowane tutaj, jeśli zapisujemy tylko genotyp w algorytmie ewolucyjnym,
            // ale dla kompletności obiektu można by je dodać. 
            // W tym kontekście wystarczy konfiguracja, bo wagi są ładowane przez setParams.
        }
    };
}