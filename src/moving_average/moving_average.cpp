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

        double average =
            (totalVolume > 0) ? weightedAveragePrice / totalVolume : 0;

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

std::vector<average_t> MovingAverage::readAveragesFromFile(long timestamp) {
    FILE* fp = fopen("data/average.txt", "r");
    if (fp == NULL) {
        std::cerr << "Error opening file" << std::endl;
        throw std::runtime_error("Failed to open average file");
    }

    std::vector<average_t> averages;
    const int MAX_LINE = 1024;
    char buffer[MAX_LINE];

    // Move to the end of the file
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);

    // Start from the end and move backwards
    long position = fileSize;

    // Read the file backwards line by line
    while (position > 0) {
        // Move back until we find a newline or reach the beginning
        long i;
        for (i = position - 1; i >= 0; i--) {
            fseek(fp, i, SEEK_SET);
            char c = fgetc(fp);
            if (c == '\n' || i == 0) {
                break;
            }
        }

        // Read the line
        fseek(fp, i == 0 ? 0 : i + 1, SEEK_SET);
        if (fgets(buffer, MAX_LINE, fp) != NULL) {
            // Parse the line
            average_t a;
            char symbol[32];  // Adjust size as needed
            sscanf(buffer, "%s %lf %ld", symbol, &a.average, &a.timestamp);
            a.symbol = std::string(symbol);

            averages.push_back(a);
        }

        // Move position to before the line we just read
        position = i;

        // Break if we've reached the start of the file
        if (i == 0) {
            break;
        }
    }

    fclose(fp);

    // Display all measurements
    // std::cout << "Measurements within the time window (" << windowMs
    //           << "ms):" << std::endl;
    // std::cout << "---------------------------------------------------"
    //           << std::endl;
    // for (const auto& meas : measurements) {
    //     std::cout << meas.instId << " " << meas.px << " " << meas.sz << " "
    //               << meas.ts << std::endl;
    // }
    // std::cout << "---------------------------------------------------"
    //           << std::endl;
    // std::cout << "Total measurements: " << measurements.size() << std::endl;

    return averages;
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
