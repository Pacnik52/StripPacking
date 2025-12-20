#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include "DataLoaderOdp.h"
#include "Err.h"
#include "utils.h"

namespace binpack {

    void DataLoaderOdp::load(std::vector<BinpackData> &IODs) {

        IODs.clear();

        int numIODs;
        int numLargeObjects;
        int W, V;
        int n;

        IODs.clear();

        std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;


        const std::filesystem::path FSP{"../data/"+filename + ".txt"};

        filesystem::directory_entry dir_entry(FSP);

        ifstream ifs(dir_entry.path().string());

        if (!ifs.is_open()) {
            return;
        }

        std::string line;

        int idx = 0;
        int numLines = (odp ? 10 : 9);
        while (std::getline(ifs, line))
        {
            idx++;
            if (idx == numLines) break;
        }

        ifs >> numIODs;
        for ( int j = 0; j < numIODs; j++ ) { // start; j < min( start + size, numIODs); j++ ) {
            BinpackData IOD;
            IOD.fileName = filename + "_" + to_string(j, 6);
            // IOD.fileName = filename + "_" + to_string(j, 6); // to_string with padding is not standard
            std::stringstream ss;
            ss << std::setw(6) << std::setfill('0') << j;
            IOD.fileName = filename + "_" + ss.str();

            if (odp) {
                ifs >> numLargeObjects;
            }
            ifs >> W >> V;
            IOD.PSizeX = W;
            IOD.PSizeY = V;
            ifs >> n;
            for ( int i = 0; i < n; i++ ) {
                int w, h;
                int box_num;
                ifs >> w >> h >> box_num;
                if (h < w) swap(h, w);
                // if (h < w) std::swap(h, w);
                IOD.BoxTypes.emplace_back(i, w, h);
                IOD.BoxToLoad.push_back(box_num);
            }
            if (j >= start && j < start + size) {
                IODs.push_back(IOD);
            }
        }
    };
}