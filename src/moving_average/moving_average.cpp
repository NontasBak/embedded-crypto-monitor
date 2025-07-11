#include "moving_average.hpp"

#include <pthread.h>

#include <chrono>
#include <iostream>
#include <mutex>

#include "../measurement/measurement.hpp"
#include "../scheduler/scheduler.hpp"

const long MovingAverage::AVERAGE_HISTORY_MS =
    60 * 60 * 1000;  // 60 minutes
std::map<std::string, std::deque<average_t>> MovingAverage::latestAverages;
pthread_mutex_t MovingAverage::averagesMutex;
int MovingAverage::window = 15 * 60 * 1000;  // 15 minutes

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
    pthread_mutex_lock(&averagesMutex);
    
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

    pthread_mutex_unlock(&averagesMutex);

    return result;
}

void* MovingAverage::workerThread(void* arg) {
    scheduler_t* scheduler = (scheduler_t*)arg;

    while (scheduler->running) {
        // Wait for work signal
        pthread_mutex_lock(&scheduler->averageMutex);
        while (!scheduler->averageWorkReady && scheduler->running) {
            pthread_cond_wait(&scheduler->averageCondition,
                              &scheduler->averageMutex);
        }

        if (!scheduler->running) {
            pthread_mutex_unlock(&scheduler->averageMutex);
            break;
        }

        scheduler->averageWorkReady = false;
        long timestamp = scheduler->currentTimestamp;
        std::vector<std::string> symbols = scheduler->SYMBOLS;
        pthread_mutex_unlock(&scheduler->averageMutex);

        calculateAverageArgs args = {symbols, timestamp};
        calculateAverage(&args);
    }

    return nullptr;
}

void* MovingAverage::calculateAverage(void* arg) {
    calculateAverageArgs* args = (calculateAverageArgs*)arg;

    for (const std::string& symbol : args->SYMBOLS) {
        // Lock
        pthread_mutex_lock(&Measurement::measurementsMutex);

        // Get recent measurements (cleanup is now handled by scheduler)
        std::vector<measurement_t> measurementsForSymbol =
            Measurement::getRecentMeasurements(symbol, window,
                                               args->timestampInMs);

        // Unlock
        pthread_mutex_unlock(&Measurement::measurementsMutex);

        double weightedAveragePrice = 0;
        double totalVolume = 0;

        for (const measurement_t& meas : measurementsForSymbol) {
            weightedAveragePrice += meas.px * meas.sz;
            totalVolume += meas.sz;
        }

        double average =
            (totalVolume > 0) ? weightedAveragePrice / totalVolume : 0;

        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();
        int delay = timestamp - args->timestampInMs;

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

    pthread_mutex_lock(&averagesMutex);

    latestAverages[symbol].push_back(avg);

    pthread_mutex_unlock(&averagesMutex);

    std::string filename = "data/average.txt";

    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    fprintf(fp, "%s %.6f %ld %d\n", symbol.c_str(), average, timestamp, delay);
    fclose(fp);
};