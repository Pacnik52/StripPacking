#pragma once
#include <string>
#include <cfloat>
#include <random>
#include <cassert>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <Eigen/Dense>
#include "../bin/BinpackData.h"
#include "../bin/CornerPoints.h"
#include "FFN.h"
#include "../Err.h"
#include "../utils.h"

namespace binpack {
    using namespace std;
    using namespace Eigen;

    template<typename AFType>
    class BinpackConstructionHeuristic {
        friend class boost::serialization::access;
    public:

        struct ConfigType {
            friend class boost::serialization::access;
            bool stripPacking = true;
            bool binPackInt = false;
            bool twoNets = false; // ODKOMENTOWANE: Wymagane przez kod
            typename AFType::Config AConf;

            ConfigType() {}

            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & stripPacking;
                ar & binPackInt;
                ar & twoNets;
            }
        };

    public:
        typedef BinpackData DataType;

        bool maximize() {
            return (Conf.stripPacking ? true : false);
        }

        int getParamsSize() {
            // POPRAWKA: AF to pojedynczy obiekt, nie kontener
            return AF.getParamsSize();
        }

        BinpackConstructionHeuristic& setParams( const double *params, int n ) {
            // POPRAWKA: AF to pojedynczy obiekt
            AF.setParams(params, n);
            return *this;
        }

        DataType::SolutionType run(const DataType &Data) {
            return pack(Data);
        }

    protected:
        ConfigType Conf;

        static const int GRID_NUM = 8;
        static const int PROPERTIES_SIZE = 25;

        int binVolInserted = 0;
        int totalVolInserted = 0;
        int totalVolLeft = 0;
        int numItems = 0;

        int x_max = 0;
        EvaListType EvaList;

        AFType AF;

        CornerPoints CP;

        vector<int> View1D;
        int gridSize = -1;

        vector<int> Q;
        vector<double> BA;

        vector<float> Properties;

        int currentNetwork = 0;
        int currentBin = 0;
        int PSizeX = -1;
    public:
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & Conf;
            ar & AF;
        }

    public:

        explicit BinpackConstructionHeuristic( const ConfigType &_Config)
            : Conf(_Config), AF( _Config.AConf) {

        }

    protected:

        DataType::SolutionType pack( const DataType &IOD ) {

            DataType::SolutionType Solution;

            if (Conf.stripPacking) {
                PSizeX = INT_MAX/2;
            } else {
                PSizeX = IOD.PSizeX;
            }

            init(IOD);

            Solution.BPV.clear();
            Solution.BPV.reserve(numItems);

            while(totalVolLeft > 0) {
                initBin(IOD);
                int selectedBoxTypeIdx;
                x_max = 0;

                do {
                    selectedBoxTypeIdx = -1;
                    double best_v = -DBL_MAX;
                    EvaListElementType best_EE, ETmp;

                    for (int i = 0; i < Q.size(); i++) {
                        if (Q[i] == 0) continue;

                        double v = evalBox(IOD, &(IOD.BoxTypes[i]), ETmp);

                        if (v > best_v) {
                            best_v = v;
                            selectedBoxTypeIdx = i;
                            best_EE = ETmp;
                        }
                    }

                    if (selectedBoxTypeIdx >= 0) {
                        auto BB = &IOD.BoxTypes[selectedBoxTypeIdx];
                        CP.Insert(BB, best_EE);

                        auto &ip = best_EE.first;
                        int sx = BB->SizeX, sy = BB->SizeY;
                        if (ip.Rotated) swap(sx, sy);
                        int maxItemPosX = ip.P1.X + sx;
                        x_max = max(x_max, maxItemPosX);

                        int y_max = ip.P1.Y + sy - 1;
                        y_max = (y_max + gridSize - gridSize/2) / gridSize - 1;
                        y_max = min((int) y_max, (int) View1D.size() - 1);

                        for ( int k = 0; k <= y_max; k++) {
                            View1D[k] = max(View1D[k], maxItemPosX);
                        }

                        Q[selectedBoxTypeIdx] -= 1;

                        Solution.BPV.emplace_back(selectedBoxTypeIdx, BinpackData::Pos(ip.P1.X, ip.P1.Y, ip.Rotated, currentBin));
                        int ar = BB->SizeX * BB->SizeY;
                        totalVolLeft -= ar;
                        binVolInserted += ar;
                        totalVolInserted += ar;
                    }
                } while (selectedBoxTypeIdx >= 0);

                currentBin++;
            }

            if (Conf.stripPacking) {
                Solution.setObj((double)totalVolInserted / (IOD.PSizeY * x_max));
            } else {
                if (Conf.binPackInt) {
                    Solution.setObj(currentBin);
                } else {
                    Solution.setObj((double)(currentBin-1) + (double)binVolInserted / (IOD.PSizeY * PSizeX));
                }
            }

            return Solution;
        }

        void init(const DataType &IOD) {
            gridSize = IOD.PSizeY / GRID_NUM;
            currentBin = 0;
            Q = IOD.BoxToLoad;
            totalVolLeft = 0;
            numItems = 0;

            BA.assign(IOD.BoxTypes.size(), 0.0);
            double PV = (double)IOD.PSizeY * IOD.PSizeY;
            for (int i = 0; i < Q.size(); i++) {
                auto &B = IOD.BoxTypes[i];
                BA[i] = (double) B.SizeX * B.SizeY / PV;
                totalVolLeft += Q[i]*(B.SizeX * B.SizeY);
                numItems += Q[i];
            }

            totalVolInserted = 0;
        }

        void initBin(const DataType &IOD)  {
            binVolInserted = 0;
            View1D.assign((IOD.PSizeY-1) / gridSize + 1, 0);
            CP = CornerPoints(PSizeX, IOD.PSizeY );

            // Wybór sieci (kod pozostawiony, mimo że AF to pojedynczy obiekt,
            // aby zachować zgodność z logiką oryginalną)
            if (Conf.stripPacking) {
                if (Conf.twoNets && totalVolLeft <= IOD.PSizeY * IOD.PSizeY) {
                    currentNetwork = 1;
                } else {
                    currentNetwork = 0;
                }
            } else {
                if (Conf.twoNets && totalVolLeft <= PSizeX * IOD.PSizeY) {
                    currentNetwork = 1;
                } else {
                    currentNetwork = 0;
                }
            }
        }

        void computeProperties(const DataType &IOD, const BinpackData::BoxType *Box, const EvaListElementType &EE) {
            Properties.clear();

            int PSizeX = CP.PSizeX;
            int PSizeY = CP.PSizeY;
            int x_max = (Conf.stripPacking ? this->x_max : PSizeX);
            double PO = (double) PSizeY * PSizeY / 8;

            for (auto v : View1D) {
                Properties.push_back(scaleTanh(0.5/(PSizeY/2.0) * (x_max - v)));
            }

            int sx = Box->SizeX, sy = Box->SizeY;
            if (EE.first.Rotated) {
                swap(sx, sy);
            }

            int DX =  x_max - (EE.first.P1.X + sx);
            double ar = 0.0;
            int num = 0;
            for (int i = 0; i < Q.size(); i++) {
                auto &B = IOD.BoxTypes[i];
                num += Q[i];
                ar += Q[i]*(B.SizeX * B.SizeY);
            }

            Properties.push_back(scaleTanh(0.5/(PO*8)  * ar));
            Properties.push_back(scaleTanh(0.5/(PO*3)  * ar));
            Properties.push_back(scaleTanh(0.5/(PO*2)  * (Box->SizeX * Box->SizeY) * Q[Box->idx] ));
            Properties.push_back(scaleTanh(0.5/10.0  * num));
            Properties.push_back(scaleTanh(0.5/4.0  * Q[Box->idx]));
            Properties.push_back(scaleTanh(0.5/((double)PSizeY/32.0) * EE.second.MismatchX));
            Properties.push_back(scaleTanh(0.5/((double)PSizeY/32.0) * EE.second.MismatchY));
            Properties.push_back((double)sx*2/PSizeY);
            Properties.push_back((double)sy*2/PSizeY);
            Properties.push_back(scaleTanh(0.5/0.25 * BA[Box->idx]));
            Properties.push_back(DX < 0 ? -1.0f : (float) (DX % sx) / (float)sx);
            Properties.push_back((float) (EE.second.DY % sy) / (float)sy);

            int dv = Box->SizeX * Box->SizeY;
            Properties.push_back( scaleTanh( 0.5/0.5 * ((double)(EE.second.VolumeIncrease - dv)) / dv));
            Properties.push_back(scaleTanh(0.5/(PSizeY/2.0) * (x_max - EE.first.P1.X)));
            Properties.push_back(((float) (PSizeY - EE.first.P1.Y)) / PSizeY);
            Properties.push_back(scaleTanh(0.5/(PSizeY/2.0) * DX));
            Properties.push_back((float) EE.second.DY / PSizeY);

            if (Properties.size() != PROPERTIES_SIZE) {
                ERROR("Wrong number of properties provided.");
            }
        }

        double evalBox(const DataType &IOD, const BinpackData::BoxType* Box, EvaListElementType &ERet) {
            EvaList.clear();
            CP.Evaluate(Box, false, EvaList);
            CP.Evaluate(Box, true, EvaList);

            if (EvaList.empty()) return -DBL_MAX;

            int best_i = -1;
            double best_eval = -DBL_MAX;

            auto View1DTmp = View1D;
            int x_max_tmp = x_max;

            for (int i = 0; i < EvaList.size(); i++) {
                auto &BB = Box;
                auto &EE = EvaList[i];
                auto &ip = EE.first;

                int sx = BB->SizeX, sy = BB->SizeY;
                if (ip.Rotated) swap(sx, sy);
                int y_max = ip.P1.Y + sy - 1;
                y_max = (y_max + gridSize - gridSize/2) / gridSize - 1;
                y_max = min((int) y_max, (int) View1D.size() - 1);
                int maxItemPosX = ip.P1.X + sx;
                x_max = max(x_max, maxItemPosX);

                for (int k = 0; k <= y_max; k++) {
                    View1D[k]  = max(View1D[k], maxItemPosX);
                }

                computeProperties(IOD, Box, EE);

                double e = AF(Properties.data(), Properties.size());

                if (e > best_eval) {
                    best_eval = e;
                    best_i = i;
                }

                View1D = View1DTmp;
                x_max = x_max_tmp;
            }

            if (best_i >= 0) {
                ERet = EvaList[best_i];
            }
            return best_eval;
        }
    };

    // static_assert usunięty, ponieważ brak pliku konceptu
    // static_assert(chof::ConstructionHeuristicConcept<BinpackConstructionHeuristic<nnutils::FFN>>);
};