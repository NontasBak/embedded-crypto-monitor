#include "pearson.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iostream>

#include "../moving_average/moving_average.hpp"

void Pearson::writePearsonToFile(std::string symbol1, std::string symbol2,
                                 double pearson, long timestamp,
                                 long maxTimestamp, int delay) {
    std::string filename = "data/pearson.txt";

    // open the file for writing
    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    // write to the text file
    fprintf(fp, "%s %s %.6f %ld %ld %d\n", symbol1.c_str(), symbol2.c_str(),
            pearson, maxTimestamp, timestamp, delay);
    fclose(fp);
}

double Pearson::calculatePearson(const std::vector<double>& x,
                                 const std::vector<double>& y) {
    if (x.size() != y.size()) {
        std::cerr << "Arrays must have the same length" << std::endl;
        return 0;
    }

    const size_t n = x.size();
    if (n == 0) {
        return 0;
    }

    // Calculate means
    double xMean = 0, yMean = 0;
    for (size_t i = 0; i < n; i++) {
        xMean += x[i];
        yMean += y[i];
    }
    xMean /= n;
    yMean /= n;

    double numerator = 0;
    double xDenominator = 0;
    double yDenominator = 0;

    for (size_t i = 0; i < n; i++) {
        double xDiff = x[i] - xMean;
        double yDiff = y[i] - yMean;
        numerator += xDiff * yDiff;
        xDenominator += xDiff * xDiff;
        yDenominator += yDiff * yDiff;
    }

    if (xDenominator == 0 || yDenominator == 0) {
        return 0;
    }

    double correlation = numerator / sqrt(xDenominator * yDenominator);
    return correlation;
}

size_t findMaximumIndex(const std::vector<double>& arr) {
    if (arr.empty()) return 0;

    double max = arr[0];
    size_t maxIndex = 0;

    for (size_t i = 1; i < arr.size(); i++) {
        if (arr[i] > max) {
            maxIndex = i;
            max = arr[i];
        }
    }

    return maxIndex;
}

void* Pearson::calculateAllPearson(void* arg) {
    calculatePearsonArgs* args = (calculatePearsonArgs*)arg;
    const long currentTimestamp = args->timestampInMs;
    const std::vector<std::string>& SYMBOLS = args->SYMBOLS;
    const int PEARSON_WINDOW = 5;

    for (const auto& symbol1 : SYMBOLS) {
        std::vector<double> averages1 =
            MovingAverage::getRecentAverages(symbol1, currentTimestamp, PEARSON_WINDOW);

        if (averages1.size() < PEARSON_WINDOW) {
            return nullptr;
        }

        std::vector<double> pearsonValues;
        std::vector<long> slidingTimestamps;
        std::vector<std::string> symbol2arr;

        for (const auto& symbol2 : SYMBOLS) {
            std::vector<double> averages2 =
                MovingAverage::getRecentAverages(symbol2, currentTimestamp);

            const size_t n = averages2.size();

            int numOfSlides;
            if (symbol1 != symbol2) {
                numOfSlides = n - PEARSON_WINDOW + 1;
            } else {
                numOfSlides = n - PEARSON_WINDOW - 1;
            }

            if (numOfSlides <= 0) continue;

            for (int i = 0; i < numOfSlides; i++) {
                std::vector<double> averagesSlice2(
                    averages2.begin() + i, averages2.begin() + i + PEARSON_WINDOW);

                double pearsonValue =
                    calculatePearson(averages1, averagesSlice2);

                pearsonValues.push_back(pearsonValue);

                // Starting timestamp of window
                const long slideTimestamp =
                    currentTimestamp - averages2.size() * 60000 + i * 60000;

                slidingTimestamps.push_back(slideTimestamp);
                symbol2arr.push_back(symbol2);
            }
        }

        // Measure delay
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();
        int delay = timestamp - currentTimestamp;

        if (!pearsonValues.empty()) {
            size_t maximumIndex = findMaximumIndex(pearsonValues);
            writePearsonToFile(symbol1, symbol2arr[maximumIndex],
                               pearsonValues[maximumIndex], currentTimestamp,
                               slidingTimestamps[maximumIndex], delay);
        }
    }

    return nullptr;
}
