#include "moving_average.hpp"

#include <pthread.h>

#include <chrono>
#include <iostream>

#include "../measurement/measurement.hpp"

int MovingAverage::window = 15 * 60 * 1000;  // In milliseconds

void* MovingAverage::calculateAverage(void* arg) {
    calculateAverageArgs* args = (calculateAverageArgs*)arg;

    std::vector<measurement_t> measurements =
        Measurement::readMeasurementsFromFile(window, args->timestampInMs);

    for (const std::string& symbol : args->SYMBOLS) {
        std::vector<measurement_t> measurementsForSymbol;

        for (const measurement_t& meas : measurements) {
            if (meas.instId == symbol) {
                measurementsForSymbol.push_back(meas);
            }
        }

        double weightedAveragePrice = 0;
        double totalVolume = 0;

        for (const measurement_t& meas : measurementsForSymbol) {
            weightedAveragePrice += meas.px * meas.sz;
            totalVolume += meas.sz;
        }

        double average = (totalVolume > 0) ? weightedAveragePrice / totalVolume : 0;

        // Measure delay
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();
        int delay = timestamp - args->timestampInMs;

        // Add results to txt file
        MovingAverage::writeAverageToFile(symbol, average, args->timestampInMs,
                                          delay);

        std::cout << "Moving average for " << symbol << ": " << average
                  << std::endl;
    }

    return nullptr;
}

void MovingAverage::writeAverageToFile(std::string symbol, double average,
                                       long timestamp, int delay) {
    std::string filename = "data/average.txt";

    // open the file for writing
    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    // write to the text file
    fprintf(fp, "%s %.6f %ld %d\n", symbol.c_str(), average, timestamp, delay);

    fclose(fp);
};
