#include "moving_average.hpp"

#include <pthread.h>

#include <chrono>
#include <iostream>

#include "../measurement/measurement.hpp"
#include "../scheduler/scheduler.hpp"

const long MovingAverage::MA_WINDOW = 15 * 60 * 1000;   // 15 minutes
const long MovingAverage::EMA_WINDOW = 28 * 60 * 1000;  // 26 minutes
const long MovingAverage::AVERAGE_HISTORY_MS =
    60 * 60 * 1000;  // 60 minutes, for pearson
const long MovingAverage::SHORT_TERM_EMA_WINDOW = 12 * 60 * 1000;  // 12 minutes
const long MovingAverage::LONG_TERM_EMA_WINDOW = 26 * 60 * 1000;   // 26 minutes
const long MovingAverage::SIGNAL_WINDOW = 9 * 60 * 1000;           // 9 minutes
std::map<std::string, std::deque<average_t>> MovingAverage::latestAverages;
std::map<std::string, std::deque<average_t>>
    MovingAverage::latestExponentialAverages;
std::map<std::string, std::deque<average_t>> MovingAverage::latestShortTermEMA;
std::map<std::string, std::deque<average_t>> MovingAverage::latestLongTermEMA;
std::map<std::string, std::deque<macd_t>> MovingAverage::latestMACD;
std::map<std::string, std::deque<signal_t>> MovingAverage::latestSignal;
std::map<std::string, std::deque<distance_t>> MovingAverage::latestDistance;
pthread_mutex_t MovingAverage::averagesMutex;

void MovingAverage::cleanupOldAverages(long currentTimestamp) {
    for (auto& pair : latestAverages) {
        std::deque<average_t>& symbolAverages = pair.second;
        while (!symbolAverages.empty() &&
               (currentTimestamp - symbolAverages.front().timestamp >
                AVERAGE_HISTORY_MS)) {
            symbolAverages.pop_front();
        }
    }
}

// void MovingAverage::cleanupOldData(long currentTimestamp) {
//     // Short-term EMA data
//     for (auto& pair : latestShortTermEMA) {
//         std::deque<average_t>& shortTermEMA = pair.second;
//         while (!shortTermEMA.empty() &&
//                (currentTimestamp - shortTermEMA.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             shortTermEMA.pop_front();
//         }
//     }

//     // long-term EMA data
//     for (auto& pair : latestLongTermEMA) {
//         std::deque<average_t>& longTermEMA = pair.second;
//         while (!longTermEMA.empty() &&
//                (currentTimestamp - longTermEMA.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             longTermEMA.pop_front();
//         }
//     }

//     // MACD data
//     for (auto& pair : latestMACD) {
//         std::deque<macd_t>& macdData = pair.second;
//         while (!macdData.empty() &&
//                (currentTimestamp - macdData.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             macdData.pop_front();
//         }
//     }

//     // signal data
//     for (auto& pair : latestSignal) {
//         std::deque<signal_t>& signalData = pair.second;
//         while (!signalData.empty() &&
//                (currentTimestamp - signalData.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             signalData.pop_front();
//         }
//     }

//     // distance data
//     for (auto& pair : latestDistance) {
//         std::deque<distance_t>& distanceData = pair.second;
//         while (!distanceData.empty() &&
//                (currentTimestamp - distanceData.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             distanceData.pop_front();
//         }
//     }
// }

std::vector<double> MovingAverage::getRecentAverages(const std::string& symbol,
                                                     long timestamp,
                                                     size_t window) {
    pthread_mutex_lock(&averagesMutex);

    std::vector<double> result;
    const std::deque<average_t>& averages = latestAverages[symbol];

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
    std::vector<std::string> symbols = scheduler->SYMBOLS;

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
        pthread_mutex_unlock(&scheduler->averageMutex);

        calculateAverage(symbols, timestamp);
        calculateAllExponentialAverages(symbols, timestamp);
        calculateMACD(symbols, timestamp);
        calculateSignal(symbols, timestamp, SIGNAL_WINDOW);
        calculateDistance(symbols, timestamp);

        // for (const std::string& symbol : symbols) {
        //     cleanupOldAverages(symbol, timestamp);
        //     cleanupOldMACDData(symbol, timestamp);
        // }
    }

    return nullptr;
}

void* MovingAverage::calculateAverage(std::vector<std::string> symbols,
                                      long currentTimestamp) {
    for (const std::string& symbol : symbols) {
        std::vector<measurement_t> measurementsForSymbol =
            Measurement::getRecentMeasurements(symbol, MA_WINDOW,
                                               currentTimestamp);

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
        int delay = timestamp - currentTimestamp;

        MovingAverage::storeAverage(symbol, average, currentTimestamp, delay,
                                    "simple");

        std::cout << "Moving average for " << symbol << ": " << average
                  << std::endl;
    }

    return nullptr;
}

void* MovingAverage::calculateAllExponentialAverages(
    std::vector<std::string> symbols, long currentTimestamp) {
    for (const std::string& symbol : symbols) {
        std::vector<measurement_t> measurementsForSymbolShortTerm =
            Measurement::getRecentMeasurements(symbol, SHORT_TERM_EMA_WINDOW,
                                               currentTimestamp);
        std::vector<measurement_t> measurementsForSymbolLongTerm =
            Measurement::getRecentMeasurements(symbol, LONG_TERM_EMA_WINDOW,
                                               currentTimestamp);

        pthread_mutex_lock(&averagesMutex);
        double previousEMAShortTerm = 0;
        double previousEMALongTerm = 0;
        if (!latestShortTermEMA[symbol].empty()) {
            previousEMAShortTerm = latestShortTermEMA[symbol].back().average;
        }
        if (!latestLongTermEMA[symbol].empty()) {
            previousEMALongTerm = latestLongTermEMA[symbol].back().average;
        }
        pthread_mutex_unlock(&averagesMutex);

        double exponentialAverageShortTerm = calculateExponentialAverage(
            measurementsForSymbolShortTerm, previousEMAShortTerm,
            SHORT_TERM_EMA_WINDOW);
        double exponentialAverageLongTerm = calculateExponentialAverage(
            measurementsForSymbolLongTerm, previousEMALongTerm,
            LONG_TERM_EMA_WINDOW);

        average_t shortTermAverage = {
            .symbol = symbol,
            .average = exponentialAverageShortTerm,
            .timestamp = currentTimestamp,
        };

        average_t longTermAverage = {
            .symbol = symbol,
            .average = exponentialAverageLongTerm,
            .timestamp = currentTimestamp,
        };

        pthread_mutex_lock(&averagesMutex);
        latestLongTermEMA[symbol].push_back(longTermAverage);
        latestShortTermEMA[symbol].push_back(shortTermAverage);
        pthread_mutex_unlock(&averagesMutex);

        // auto now = std::chrono::system_clock::now();
        // auto timestamp =
        // std::chrono::duration_cast<std::chrono::milliseconds>(
        //                      now.time_since_epoch())
        //                      .count();
        // int delay = timestamp - currentTimestamp;

        // MovingAverage::storeAverage(symbol, exponentialAverageShortTerm,
        //                             currentTimestamp, delay,
        //                             "exponential");
        // MovingAverage::storeAverage(symbol, exponentialAverageLongTerm,
        //                             currentTimestamp, delay,
        //                             "exponential");

        std::cout << "Exponential moving average (short term) for " << symbol
                  << ": " << exponentialAverageShortTerm << std::endl;
        std::cout << "Exponential moving average (long term) for " << symbol
                  << ": " << exponentialAverageLongTerm << std::endl;
    }

    return nullptr;
}

double MovingAverage::calculateExponentialAverage(
    std::vector<measurement_t> measurements, const double previousEMA,
    long window) {
    const int n = window / (6 * 10000);
    const double alpha = 2.0 / (n + 1);

    if (measurements.empty()) {
        return 0;
    }

    double weightedAveragePrice = 0;
    double totalVolume = 0;

    for (const measurement_t& meas : measurements) {
        weightedAveragePrice += meas.px * meas.sz;
        totalVolume += meas.sz;
    }

    double currentPrice =
        (totalVolume > 0) ? weightedAveragePrice / totalVolume : 0;

    double exponentialAverage;
    if (previousEMA == 0) {
        exponentialAverage = currentPrice;
    } else {
        exponentialAverage = (currentPrice - previousEMA) * alpha + previousEMA;
    }

    return exponentialAverage;
}

void* MovingAverage::calculateMACD(std::vector<std::string> symbols,
                                   long currentTimestamp) {
    for (const std::string& symbol : symbols) {
        pthread_mutex_lock(&averagesMutex);

        double shortTermEMA = 0;
        double longTermEMA = 0;

        if (!latestShortTermEMA[symbol].empty()) {
            shortTermEMA = latestShortTermEMA[symbol].back().average;
        }

        if (!latestLongTermEMA[symbol].empty()) {
            longTermEMA = latestLongTermEMA[symbol].back().average;
        }

        pthread_mutex_unlock(&averagesMutex);

        if (shortTermEMA != 0 && longTermEMA != 0) {
            double macdValue = shortTermEMA - longTermEMA;

            macd_t macd;
            macd.symbol = symbol;
            macd.macd = macdValue;
            macd.timestamp = currentTimestamp;

            pthread_mutex_lock(&averagesMutex);
            latestMACD[symbol].push_back(macd);
            pthread_mutex_unlock(&averagesMutex);

            std::cout << "MACD for " << symbol << ": " << macdValue
                      << std::endl;
        }
    }

    return nullptr;
}

void* MovingAverage::calculateSignal(std::vector<std::string> symbols,
                                     long currentTimestamp, long window) {
    const int n = window / (6 * 10000);
    const double alpha = 2.0 / (n + 1);

    for (const std::string& symbol : symbols) {
        pthread_mutex_lock(&averagesMutex);

        double currentMACD = 0;
        if (!latestMACD[symbol].empty()) {
            currentMACD = latestMACD[symbol].back().macd;
        }

        double previousSignal = 0;
        if (!latestSignal[symbol].empty()) {
            previousSignal = latestSignal[symbol].back().signal;
        }

        pthread_mutex_unlock(&averagesMutex);

        if (currentMACD != 0) {
            double signalValue;
            if (previousSignal == 0) {
                signalValue = currentMACD;
            } else {
                signalValue =
                    (currentMACD - previousSignal) * alpha + previousSignal;
            }

            signal_t signal;
            signal.symbol = symbol;
            signal.signal = signalValue;
            signal.timestamp = currentTimestamp;

            pthread_mutex_lock(&averagesMutex);
            latestSignal[symbol].push_back(signal);
            pthread_mutex_unlock(&averagesMutex);

            std::cout << "Signal for " << symbol << ": " << signalValue
                      << std::endl;
        }
    }

    return nullptr;
}

void* MovingAverage::calculateDistance(std::vector<std::string> symbols,
                                       long currentTimestamp) {
    for (const std::string& symbol : symbols) {
        pthread_mutex_lock(&averagesMutex);

        double currentMACD = 0;
        double currentSignal = 0;

        if (!latestMACD[symbol].empty()) {
            currentMACD = latestMACD[symbol].back().macd;
        }

        if (!latestSignal[symbol].empty()) {
            currentSignal = latestSignal[symbol].back().signal;
        }

        pthread_mutex_unlock(&averagesMutex);

        if (currentMACD != 0 && currentSignal != 0) {
            double distanceValue = currentMACD - currentSignal;

            distance_t distance;
            distance.symbol = symbol;
            distance.distance = distanceValue;
            distance.timestamp = currentTimestamp;

            pthread_mutex_lock(&averagesMutex);
            latestDistance[symbol].push_back(distance);
            pthread_mutex_unlock(&averagesMutex);

            std::cout << "Distance for " << symbol << ": " << distanceValue
                      << std::endl;
        }
    }

    return nullptr;
}

void MovingAverage::storeAverage(std::string symbol, double average,
                                 long timestamp, int delay, std::string type) {
    average_t avg;
    avg.symbol = symbol;
    avg.average = average;
    avg.timestamp = timestamp;

    pthread_mutex_lock(&averagesMutex);

    if (type == "exponential") {
        latestExponentialAverages[symbol].push_back(avg);
    } else if (type == "simple") {
        latestAverages[symbol].push_back(avg);
    }

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

std::vector<double> MovingAverage::getRecentMACD(const std::string& symbol,
                                                 long timestamp,
                                                 size_t window) {
    pthread_mutex_lock(&averagesMutex);
    std::vector<double> result;
    const std::deque<macd_t>& macdData = latestMACD[symbol];

    size_t startPos =
        macdData.size() > window && window > 0 ? macdData.size() - window : 0;

    for (size_t i = startPos; i < macdData.size(); i++) {
        result.push_back(macdData.at(i).macd);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

std::vector<double> MovingAverage::getRecentSignal(const std::string& symbol,
                                                   long timestamp,
                                                   size_t window) {
    pthread_mutex_lock(&averagesMutex);

    std::vector<double> result;
    const std::deque<signal_t>& signalData = latestSignal[symbol];

    size_t startPos = signalData.size() > window && window > 0
                          ? signalData.size() - window
                          : 0;
    for (size_t i = startPos; i < signalData.size(); i++) {
        result.push_back(signalData.at(i).signal);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

std::vector<double> MovingAverage::getRecentDistance(const std::string& symbol,
                                                     long timestamp,
                                                     size_t window) {
    pthread_mutex_lock(&averagesMutex);

    std::vector<double> result;
    const std::deque<distance_t>& distanceData = latestDistance[symbol];

    size_t startPos = distanceData.size() > window && window > 0
                          ? distanceData.size() - window
                          : 0;
    for (size_t i = startPos; i < distanceData.size(); i++) {
        result.push_back(distanceData.at(i).distance);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}
