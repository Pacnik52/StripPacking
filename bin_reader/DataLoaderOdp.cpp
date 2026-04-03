#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <iomanip>
#include "DataLoaderOdp.h"
#include "../Err.h"

namespace binpack {
    void DataLoaderOdp::load(const std::string &filename, std::vector<BinpackData> &IODs, bool odp, int start,
                             int size) {
        IODs.clear();
        int numIODs;
        int numLargeObjects;
        int W, V;
        int n;
        std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
        const std::filesystem::path FSP{"../data/" + filename + ".txt"};
        std::ifstream ifs(FSP);
        if (!ifs.is_open()) {
            return;
        }

        std::string line;

        int idx = 0;
        int numLines = (odp ? 10 : 9);
        while (std::getline(ifs, line)) {
            idx++;
            if (idx == numLines) break;
        }
        ifs >> numIODs;
        for (int j = 0; j < numIODs; j++) {
            BinpackData IOD;
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
            for (int i = 0; i < n; i++) {
                int w, h;
                int box_num;
                ifs >> w >> h >> box_num;
                if (h < w) std::swap(h, w);
                IOD.BoxTypes.emplace_back(i, w, h);
                IOD.BoxToLoad.push_back(box_num);
            }
            if (j >= start && j < start + size) {
                IODs.push_back(IOD);
            }
        }
    }

    void DataLoaderOdp::loadFromMultipleFiles(
        const std::vector<std::string> &filenames,
        int totalTasks,
        std::vector<BinpackData> &IODs,
        bool odp
    ) {
        IODs.clear();
        int filesCount = filenames.size();
        if (filesCount == 0 || totalTasks == 0) return;
        int tasksPerFile = totalTasks / filesCount;
        int remainder = totalTasks % filesCount;
        for (int i = 0; i < filesCount; ++i) {
            int tasksToRead = tasksPerFile + (i < remainder ? 1 : 0);
            if (tasksToRead == 0) continue;
            std::vector<BinpackData> tempIODs;
            DataLoaderOdp::load(filenames[i], tempIODs, odp, 0, tasksToRead);
            for (auto &iod: tempIODs) {
                if ((int) IODs.size() < totalTasks)
                    IODs.push_back(iod);
            }
            if ((int) IODs.size() >= totalTasks) break;
        }
    }

    void DataLoaderOdp::loadTrainAndValidationFromMultipleFiles(
        const std::vector<std::string> &filenames,
        std::vector<BinpackData> &trainSet,
        int trainSize,
        std::vector<BinpackData> &valSet,
        int valSize,
        bool odp
    ) {
        trainSet.clear();
        valSet.clear();
        int filesCount = filenames.size();
        if (filesCount == 0) return;

        int trainPerFile = trainSize / filesCount;
        int trainRemainder = trainSize % filesCount;

        int valPerFile = valSize / filesCount;
        int valRemainder = valSize % filesCount;

        int trainSoFar = 0, valSoFar = 0;
        for (int i = 0; i < filesCount; ++i) {
            int tCount = trainPerFile + (i < trainRemainder ? 1 : 0);
            int vCount = valPerFile + (i < valRemainder ? 1 : 0);
            std::vector<BinpackData> tempTrain, tempVal;
            if (tCount > 0) {
                DataLoaderOdp::load(filenames[i], tempTrain, odp, 0, tCount);
                int toAdd = std::min(tCount, trainSize - trainSoFar);
                trainSet.insert(trainSet.end(), tempTrain.begin(), tempTrain.begin() + toAdd);
                trainSoFar += toAdd;
            }
            if (vCount > 0) {
                DataLoaderOdp::load(filenames[i], tempVal, odp, tCount, vCount);
                int toAdd = std::min(vCount, valSize - valSoFar);
                valSet.insert(valSet.end(), tempVal.begin(), tempVal.begin() + toAdd);
                valSoFar += toAdd;
            }
            if (trainSoFar >= trainSize && valSoFar >= valSize) break;
        }
    }
}
