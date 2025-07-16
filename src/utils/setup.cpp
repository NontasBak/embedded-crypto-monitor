#include "setup.hpp"

#include <errno.h>
#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <iostream>

const std::string Setup::dataPath = "data/";
const std::string Setup::files[] = {
    "meas_BTC-USDT.txt",  "meas_ADA-USDT.txt", "meas_ETH-USDT.txt",
    "meas_DOGE-USDT.txt", "meas_XRP-USDT.txt", "meas_SOL-USDT.txt",
    "meas_LTC-USDT.txt",  "meas_BNB-USDT.txt", "average.txt",
    "pearson.txt",        "cpu_stats.txt"};

void Setup::initializeFiles() {
    int status = mkdir(dataPath.c_str(), 0777);

    if (status != 0 && errno != EEXIST) {
        std::cerr << "Error creating directory " << dataPath << ": "
                  << strerror(errno) << std::endl;
    } else {
        for (const std::string& file : files) {
            std::string filePath = dataPath + file;
            std::ofstream outFile(filePath.c_str());

            if (outFile.is_open()) {
                outFile.close();
                // std::cout << "Created file: " << filePath << std::endl;
            } else {
                std::cerr << "Failed to create file: " << filePath << ": "
                          << strerror(errno) << std::endl;
                return;
            }
        }
        std::cout << "Created files successfully." << std::endl;
    }
}
