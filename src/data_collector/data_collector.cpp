#include <pthread.h>

#include <chrono>
#include <iostream>

#include "../measurement/measurement.hpp"
#include "../scheduler/scheduler.hpp"
#include "data_collector.hpp"

const long DataCollector::MA_WINDOW = 15 * 60 * 1000;   // 15 minutes
const long DataCollector::EMA_WINDOW = 28 * 60 * 1000;  // 26 minutes
const long DataCollector::AVERAGE_HISTORY_MS =
    60 * 60 * 1000;  // 60 minutes, for pearson
const long DataCollector::SHORT_TERM_EMA_WINDOW = 12 * 60 * 1000;  // 12 minutes
const long DataCollector::LONG_TERM_EMA_WINDOW = 26 * 60 * 1000;   // 26 minutes
const long DataCollector::SIGNAL_WINDOW = 9 * 60 * 1000;           // 9 minutes
std::map<std::string, std::deque<dataPoint_t>> DataCollector::latestAverages;
std::map<std::string, std::deque<dataPoint_t>>
    DataCollector::latestExponentialAverages;
std::map<std::string, std::deque<dataPoint_t>>
    DataCollector::latestShortTermEMA;
std::map<std::string, std::deque<dataPoint_t>> DataCollector::latestLongTermEMA;
std::map<std::string, std::deque<dataPoint_t>> DataCollector::latestMACD;
std::map<std::string, std::deque<dataPoint_t>> DataCollector::latestSignal;
std::map<std::string, std::deque<dataPoint_t>> DataCollector::latestDistance;
std::map<std::string, std::deque<dataPoint_t>>
    DataCollector::latestClosingPrices;
pthread_mutex_t DataCollector::averagesMutex;

void* DataCollector::workerThread(void* arg) {
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
        calculateClosingPrice(symbols, timestamp);

        // for (const std::string& symbol : symbols) {
        //     cleanupOldAverages(symbol, timestamp);
        //     cleanupOldMACDData(symbol, timestamp);
        // }
    }

    return nullptr;
}

void* DataCollector::calculateAverage(std::vector<std::string> symbols,
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

        DataCollector::storeAverage(symbol, average, currentTimestamp, delay,
                                    "simple");

        std::cout << "Moving average for " << symbol << ": " << average
                  << std::endl;
    }

    return nullptr;
}

void* DataCollector::calculateAllExponentialAverages(
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
            previousEMAShortTerm = latestShortTermEMA[symbol].back().data;
        }
        if (!latestLongTermEMA[symbol].empty()) {
            previousEMALongTerm = latestLongTermEMA[symbol].back().data;
        }
        pthread_mutex_unlock(&averagesMutex);

        double exponentialAverageShortTerm = calculateExponentialAverage(
            measurementsForSymbolShortTerm, previousEMAShortTerm,
            SHORT_TERM_EMA_WINDOW);
        double exponentialAverageLongTerm = calculateExponentialAverage(
            measurementsForSymbolLongTerm, previousEMALongTerm,
            LONG_TERM_EMA_WINDOW);

        dataPoint_t shortTermAverage = {
            .data = exponentialAverageShortTerm,
            .timestamp = currentTimestamp,
        };

        dataPoint_t longTermAverage = {
            .data = exponentialAverageLongTerm,
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

        // DataCollector::storeAverage(symbol, exponentialAverageShortTerm,
        //                             currentTimestamp, delay,
        //                             "exponential");
        // DataCollector::storeAverage(symbol, exponentialAverageLongTerm,
        //                             currentTimestamp, delay,
        //                             "exponential");

        std::cout << "Exponential moving average (short term) for " << symbol
                  << ": " << exponentialAverageShortTerm << std::endl;
        std::cout << "Exponential moving average (long term) for " << symbol
                  << ": " << exponentialAverageLongTerm << std::endl;
    }

    return nullptr;
}

double DataCollector::calculateExponentialAverage(
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

void* DataCollector::calculateMACD(std::vector<std::string> symbols,
                                   long currentTimestamp) {
    for (const std::string& symbol : symbols) {
        pthread_mutex_lock(&averagesMutex);

        double shortTermEMA = 0;
        double longTermEMA = 0;

        if (!latestShortTermEMA[symbol].empty()) {
            shortTermEMA = latestShortTermEMA[symbol].back().data;
        }

        if (!latestLongTermEMA[symbol].empty()) {
            longTermEMA = latestLongTermEMA[symbol].back().data;
        }

        pthread_mutex_unlock(&averagesMutex);

        if (shortTermEMA != 0 && longTermEMA != 0) {
            double macdValue = shortTermEMA - longTermEMA;

            dataPoint_t macd = {.data = macdValue,
                                .timestamp = currentTimestamp};

            pthread_mutex_lock(&averagesMutex);
            latestMACD[symbol].push_back(macd);
            pthread_mutex_unlock(&averagesMutex);

            std::cout << "MACD for " << symbol << ": " << macdValue
                      << std::endl;
        }
    }

    return nullptr;
}

void* DataCollector::calculateSignal(std::vector<std::string> symbols,
                                     long currentTimestamp, long window) {
    const int n = window / (6 * 10000);
    const double alpha = 2.0 / (n + 1);

    for (const std::string& symbol : symbols) {
        pthread_mutex_lock(&averagesMutex);

        double currentMACD = 0;
        if (!latestMACD[symbol].empty()) {
            currentMACD = latestMACD[symbol].back().data;
        }

        double previousSignal = 0;
        if (!latestSignal[symbol].empty()) {
            previousSignal = latestSignal[symbol].back().data;
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

            dataPoint_t signal = {.data = signalValue,
                                  .timestamp = currentTimestamp};

            pthread_mutex_lock(&averagesMutex);
            latestSignal[symbol].push_back(signal);
            pthread_mutex_unlock(&averagesMutex);

            std::cout << "Signal for " << symbol << ": " << signalValue
                      << std::endl;
        }
    }

    return nullptr;
}

void* DataCollector::calculateDistance(std::vector<std::string> symbols,
                                       long currentTimestamp) {
    for (const std::string& symbol : symbols) {
        pthread_mutex_lock(&averagesMutex);

        double currentMACD = 0;
        double currentSignal = 0;

        if (!latestMACD[symbol].empty()) {
            currentMACD = latestMACD[symbol].back().data;
        }

        if (!latestSignal[symbol].empty()) {
            currentSignal = latestSignal[symbol].back().data;
        }

        pthread_mutex_unlock(&averagesMutex);

        if (currentMACD != 0 && currentSignal != 0) {
            double distanceValue = currentMACD - currentSignal;

            dataPoint_t distance = {.data = distanceValue,
                                    .timestamp = currentTimestamp};

            pthread_mutex_lock(&averagesMutex);
            latestDistance[symbol].push_back(distance);
            pthread_mutex_unlock(&averagesMutex);

            std::cout << "Distance for " << symbol << ": " << distanceValue
                      << std::endl;
        }
    }

    return nullptr;
}

void* DataCollector::calculateClosingPrice(std::vector<std::string> symbols,
                                           long currentTimestamp) {
    for (const std::string& symbol : symbols) {
        std::vector<measurement_t> measurementsForSymbol =
            Measurement::getRecentMeasurements(symbol, 60 * 1000,
                                               currentTimestamp);

        if (!measurementsForSymbol.empty()) {
            // Get the most recent measurement as the closing price
            measurement_t latestMeasurement = measurementsForSymbol.back();
            double closingPrice = latestMeasurement.px;

            dataPoint_t closingPricePoint = {.data = closingPrice,
                                             .timestamp = currentTimestamp};

            pthread_mutex_lock(&averagesMutex);
            latestClosingPrices[symbol].push_back(closingPricePoint);
            pthread_mutex_unlock(&averagesMutex);

            std::cout << "Closing price for " << symbol << ": " << closingPrice
                      << std::endl;
        }
    }

    return nullptr;
}

void DataCollector::storeAverage(std::string symbol, double average,
                                 long timestamp, int delay, std::string type) {
    dataPoint_t avg = {.data = average, .timestamp = timestamp};

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

value_t DataCollector::getRecentEMA(const std::string& symbol, long timestamp,
                                    size_t window, std::string type) {
    pthread_mutex_lock(&averagesMutex);

    value_t result;
    const std::deque<dataPoint_t>* averages;
    if (type == "short") {
        averages = &latestShortTermEMA[symbol];
    } else if (type == "long") {
        averages = &latestLongTermEMA[symbol];
    } else {
        std::cerr << "Invalid type: " << type << std::endl;
        pthread_mutex_unlock(&averagesMutex);
        return result;
    }

    size_t startPos =
        averages->size() > window && window > 0 ? averages->size() - window : 0;
    for (size_t i = startPos; i < averages->size(); i++) {
        result.values.push_back(averages->at(i).data);
        result.timestamps.push_back(averages->at(i).timestamp);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

value_t DataCollector::getRecentAverages(const std::string& symbol,
                                         long timestamp, size_t window) {
    pthread_mutex_lock(&averagesMutex);

    value_t result;
    const std::deque<dataPoint_t>& averages = latestAverages[symbol];

    size_t startPos =
        averages.size() > window && window > 0 ? averages.size() - window : 0;
    for (size_t i = startPos; i < averages.size(); i++) {
        result.values.push_back(averages.at(i).data);
        result.timestamps.push_back(averages.at(i).timestamp);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

value_t DataCollector::getRecentMACD(const std::string& symbol, long timestamp,
                                     size_t window) {
    pthread_mutex_lock(&averagesMutex);
    value_t result;
    const std::deque<dataPoint_t>& macdData = latestMACD[symbol];

    size_t startPos =
        macdData.size() > window && window > 0 ? macdData.size() - window : 0;

    for (size_t i = startPos; i < macdData.size(); i++) {
        result.values.push_back(macdData.at(i).data);
        result.timestamps.push_back(macdData.at(i).timestamp);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

value_t DataCollector::getRecentSignal(const std::string& symbol,
                                       long timestamp, size_t window) {
    pthread_mutex_lock(&averagesMutex);

    value_t result;
    const std::deque<dataPoint_t>& signalData = latestSignal[symbol];

    size_t startPos = signalData.size() > window && window > 0
                          ? signalData.size() - window
                          : 0;
    for (size_t i = startPos; i < signalData.size(); i++) {
        result.values.push_back(signalData.at(i).data);
        result.timestamps.push_back(signalData.at(i).timestamp);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

value_t DataCollector::getRecentDistance(const std::string& symbol,
                                         long timestamp, size_t window) {
    pthread_mutex_lock(&averagesMutex);

    value_t result;
    const std::deque<dataPoint_t>& distanceData = latestDistance[symbol];

    size_t startPos = distanceData.size() > window && window > 0
                          ? distanceData.size() - window
                          : 0;
    for (size_t i = startPos; i < distanceData.size(); i++) {
        result.values.push_back(distanceData.at(i).data);
        result.timestamps.push_back(distanceData.at(i).timestamp);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

value_t DataCollector::getRecentClosingPrices(const std::string& symbol,
                                              long timestamp, size_t window) {
    pthread_mutex_lock(&averagesMutex);

    value_t result;
    const std::deque<dataPoint_t>& closingPriceData =
        latestClosingPrices[symbol];

    size_t startPos = closingPriceData.size() > window && window > 0
                          ? closingPriceData.size() - window
                          : 0;
    for (size_t i = startPos; i < closingPriceData.size(); i++) {
        result.values.push_back(closingPriceData.at(i).data);
        result.timestamps.push_back(closingPriceData.at(i).timestamp);
    }
    pthread_mutex_unlock(&averagesMutex);

    return result;
}

void DataCollector::cleanupOldAverages(long currentTimestamp) {
    for (auto& pair : latestAverages) {
        std::deque<dataPoint_t>& symbolAverages = pair.second;
        while (!symbolAverages.empty() &&
               (currentTimestamp - symbolAverages.front().timestamp >
                AVERAGE_HISTORY_MS)) {
            symbolAverages.pop_front();
        }
    }
}

// void DataCollector::cleanupOldData(long currentTimestamp) {
//     // Short-term EMA data
//     for (auto& pair : latestShortTermEMA) {
//         std::deque<dataPoint_t>& shortTermEMA = pair.second;
//         while (!shortTermEMA.empty() &&
//                (currentTimestamp - shortTermEMA.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             shortTermEMA.pop_front();
//         }
//     }

//     // long-term EMA data
//     for (auto& pair : latestLongTermEMA) {
//         std::deque<dataPoint_t>& longTermEMA = pair.second;
//         while (!longTermEMA.empty() &&
//                (currentTimestamp - longTermEMA.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             longTermEMA.pop_front();
//         }
//     }

//     // MACD data
//     for (auto& pair : latestMACD) {
//         std::deque<dataPoint_t>& macdData = pair.second;
//         while (!macdData.empty() &&
//                (currentTimestamp - macdData.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             macdData.pop_front();
//         }
//     }

//     // signal data
//     for (auto& pair : latestSignal) {
//         std::deque<dataPoint_t>& signalData = pair.second;
//         while (!signalData.empty() &&
//                (currentTimestamp - signalData.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             signalData.pop_front();
//         }
//     }

//     // distance data
//     for (auto& pair : latestDistance) {
//         std::deque<dataPoint_t>& distanceData = pair.second;
//         while (!distanceData.empty() &&
//                (currentTimestamp - distanceData.front().timestamp >
//                 AVERAGE_HISTORY_MS)) {
//             distanceData.pop_front();
//         }
//     }
// }
