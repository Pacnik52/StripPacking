//
// Created by tsliwins on 02.12.24.
//

#pragma once
#include <vector>
#include <string>
#include "BinpackData.h"
#include "utils.h"

namespace binpack {
    using namespace std;
    //using namespace __gnu_debug;
    struct DataLoaderOdp {
        string filename;
        int start, size;
        bool odp;

        DataLoaderOdp( string _filename, bool _odp, int _start = 0, int _size = -1 )
        : filename( _filename ), start( _start ), size( _size ), odp( _odp ) {

        }

        //    vector<PalOpt::InputOutputData> IODs;
        void load(vector<BinpackData> &IODs);

        static string composeFileName( string dir, string prefix, int n, pair<int, int> Range, int id ) {
            return dir
                + "/"
                + prefix
                + std::to_string(n)
                +  "_" + std::to_string(Range.first) + "-" + std::to_string(Range.second)
                + "_" + std::to_string(id);
        }
    };
}

