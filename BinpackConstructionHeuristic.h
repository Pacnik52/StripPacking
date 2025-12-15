#pragma once
#include <string>
#include <cfloat>
#include <random>
#include <cassert>
// NOTE: The following headers are for an advanced ML-based heuristic and are not part of this simplified project.
// They have been commented out to allow the project to compile.
// #include <boost/archive/binary_oarchive.hpp>
// #include <boost/archive/binary_iarchive.hpp>
// #include <Eigen/Dense>
// #include "ConstructionHeuristicConcept.h"
#include "BinpackData.h"
//#include "ReadConfig.h"
#include "CornerPoints.h"
// #include "FFN.h"
#include "Err.h"
#include "utils.h"


namespace binpack {
    using namespace std;
    // using namespace ReadConfig;
    // using namespace Eigen;


    template<typename AFType>
    class BinpackConstructionHeuristic {
        // friend class boost::serialization::access;
    public:

        struct ConfigType {
            // friend class boost::serialization::access;
            bool stripPacking;
            bool binPackInt = false;
      //      bool twoNets;
            // typename AFType::Config AConf;

            ConfigType() {

            }

            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & stripPacking;
                ar & binPackInt;
            }

        };
    // ... rest of the file is unchanged, but will not compile if used ...
    // ... because AFType, Eigen, etc. are not defined. ...
};

// static_assert(chof::ConstructionHeuristicConcept<BinpackConstructionHeuristic<nnutils::FFN>>);
};
