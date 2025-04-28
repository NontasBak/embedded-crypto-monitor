#include "moving_average.hpp"

#include <pthread.h>

#include <chrono>
#include <iostream>
#include <mutex>

#include "../measurement/measurement.hpp"

const long MovingAverage::AVERAGE_HISTORY_MS =
    60 * 60 * 1000;  // 60 minutes in milliseconds
std::map<std::string, std::deque<average_t>> MovingAverage::latestAverages;
std::mutex MovingAverage::averagesMutex;
int MovingAverage::window = 15 * 60 * 1000;  // 15 minutes in milliseconds

void MovingAverage::cleanupOldAverages(const std::string& symbol,
                                       long currentTimestamp) {
    std::deque<average_t>& symbolAverages = latestAverages[symbol];
    while (!symbolAverages.empty() &&
           (currentTimestamp - symbolAverages.front().timestamp >
            AVERAGE_HISTORY_MS)) {
        symbolAverages.pop_front();
    }
}

std::vector<double> MovingAverage::getRecentAverages(const std::string& symbol,
                                                     long timestamp,
                                                     size_t window) {
    std::vector<double> result;

    const std::deque<average_t>& averages = latestAverages[symbol];

    // Determine starting position based on window size
    // The latest values are at the end of the deque
    // If window is 0, return all values
    size_t startPos =
        averages.size() > window && window > 0 ? averages.size() - window : 0;

    for (size_t i = startPos; i < averages.size(); i++) {
        result.push_back(averages.at(i).average);
    }

    return result;
}

void* MovingAverage::calculateAverage(void* arg) {
    calculateAverageArgs* args = (calculateAverageArgs*)arg;

    for (const std::string& symbol : args->SYMBOLS) {
        // Lock
        std::lock_guard<std::mutex> lock(Measurement::measurementsMutex);

        // Cleanup and get recent measurements
        Measurement::cleanupOldMeasurements(symbol, args->timestampInMs);
        std::vector<measurement_t> measurementsForSymbol =
            Measurement::getRecentMeasurements(symbol, window,
                                               args->timestampInMs);

        // Unlock
        Measurement::measurementsMutex.unlock();

        double weightedAveragePrice = 0;
        double totalVolume = 0;

        for (const measurement_t& meas : measurementsForSymbol) {
            weightedAveragePrice += meas.px * meas.sz;
            totalVolume += meas.sz;
        }

        double average =
            (totalVolume > 0) ? weightedAveragePrice / totalVolume : 0;

        // Measure delay
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();
        int delay = timestamp - args->timestampInMs;

        // Add results to memory and txt file
        MovingAverage::storeAverage(symbol, average, args->timestampInMs,
                                    delay);

        std::cout << "Moving average for " << symbol << ": " << average
                  << std::endl;
    }

    return nullptr;
}

void MovingAverage::storeAverage(std::string symbol, double average,
                                 long timestamp, int delay) {
    average_t avg;
    avg.symbol = symbol;
    avg.average = average;
    avg.timestamp = timestamp;

    latestAverages[symbol].push_back(avg);

    // Write to file
    std::string filename = "data/average.txt";

    // Open the file for writing
    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    // Write to the text file
    fprintf(fp, "%s %.6f %ld %d\n", symbol.c_str(), average, timestamp, delay);

    fclose(fp);
};
