extern "C" {
    #include "cmaes.h"
}

extern "C" {
    double * cmaes_init(cmaes_t *, int, double *, double *, long, int, const char *);
    void cmaes_exit(cmaes_t *);
    double * const * cmaes_SamplePopulation(cmaes_t *);
    double * cmaes_UpdateDistribution(cmaes_t *, const double *);
    const double * cmaes_GetPtr(cmaes_t *, const char *);
    double cmaes_Get(cmaes_t *, const char *);
    int cmaes_TestForTermination(cmaes_t *);
    char * cmaes_SayHello(cmaes_t *);
}

#include "Trainer.h"
#include "NeuralStrategy.h"
#include <iostream>
#include <vector>

std::vector<binpack::BinpackData>* Trainer::trainingData = nullptr;

double fitfunc_wrapper(double const *x, int dim) {
    if (!Trainer::trainingData) return 0.0;

    NeuralNetwork net;
    net.setWeights(x);

    NeuralStrategy strategy(net);
    double totalFillFactor = 0;
    int count = 0;

    for(int i = 0; i < (int)Trainer::trainingData->size(); ++i) {
        binpack::BinpackData instance = (*Trainer::trainingData)[i];
        double ff = strategy.solve(instance);
        totalFillFactor += ff;
        count++;
    }

    double avgFillFactor = (count > 0) ? (totalFillFactor / count) : 0.0;

    return -avgFillFactor;
}

void Trainer::train(std::vector<binpack::BinpackData>& data, int generations) {
    Trainer::trainingData = &data;

    cmaes_t evo;
    NeuralNetwork dummy;
    int dim = dummy.getParameterCount();

    std::vector<double> xstart(dim, 0.0);
    std::vector<double> stddev(dim, 0.5);

    double* arFunvals;

    arFunvals = cmaes_init(&evo, dim, xstart.data(), stddev.data(), 0, 0, NULL);

    std::cout << "Starting CMA-ES training..." << std::endl;
    std::cout << cmaes_SayHello(&evo) << std::endl;

    int iter = 0;
    while(!cmaes_TestForTermination(&evo) && iter < generations) {

        double *const *pop = cmaes_SamplePopulation(&evo);
        int popSize = (int)cmaes_Get(&evo, "lambda");

        for(int i = 0; i < popSize; ++i) {
            arFunvals[i] = fitfunc_wrapper(pop[i], dim);
        }

        cmaes_UpdateDistribution(&evo, arFunvals);

        if (iter % 1 == 0) {
            double bestScore = cmaes_Get(&evo, "fbestever");
            std::cout << "Gen " << iter << ": Best FF = " << (-bestScore * 100.0) << "%" << std::endl;
        }
        iter++;
    }

    const double* bestWeights = cmaes_GetPtr(&evo, "xbestever");
    bestNet.setWeights(bestWeights);

    cmaes_exit(&evo);
    Trainer::trainingData = nullptr;
}

void Trainer::saveBest(const std::string& filename) {
    return;
}