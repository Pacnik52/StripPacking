#pragma once
#include <vector>
#include <string>
#include "../bin/BinpackData.h"
#include "../utils.h"

namespace binpack {
    using namespace std;

    struct DataLoaderOdp {
        static void load(const std::string &_filename, std::vector<BinpackData> &IODs, bool odp, int _start = 0,
                         int _size = -1);

        static string composeFileName(string dir, string prefix, int n, pair<int, int> Range, int id) {
            return dir
                   + "/"
                   + prefix
                   + std::to_string(n)
                   + "_" + std::to_string(Range.first) + "-" + std::to_string(Range.second)
                   + "_" + std::to_string(id);
        }

        static void loadFromMultipleFiles(const std::vector<std::string> &filenames, int totalTasks,
                                          std::vector<BinpackData> &IODs, bool odp
        );

        static void loadTrainAndValidationFromMultipleFiles(const std::vector<std::string> &filenames,
                                                            std::vector<BinpackData> &trainSet, int trainSize,
                                                            std::vector<BinpackData> &valSet, int valSize, bool odp);
    };
}
