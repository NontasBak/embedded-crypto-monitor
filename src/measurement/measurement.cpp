#include "measurement.hpp"

#include <cstddef>
#include <cstdio>
#include <deque>
#include <iostream>
#include <map>
#include <mutex>

// Initialize in-memory storage
std::map<std::string, std::deque<measurement_t>>
    Measurement::latestMeasurements;
std::mutex Measurement::measurementsMutex;
const long Measurement::MEASUREMENT_WINDOW_MS =
    15 * 60 * 1000;  // 15 minutes in milliseconds

measurement_t Measurement::create(const std::string instId, double px,
                                  double sz, long ts) {
    measurement_t m;
    m.instId = instId;
    m.px = px;
    m.sz = sz;
    m.ts = ts;
    return m;
};

void Measurement::displayMeasurement(const measurement_t& m) {
    std::cout << m.instId << " " << m.px << " " << m.sz << " " << m.ts
              << std::endl;
};

void Measurement::cleanupOldMeasurements(const std::string& symbol,
                                         long currentTimestamp) {
    std::deque<measurement_t>& symbolMeasurements = latestMeasurements[symbol];
    while (!symbolMeasurements.empty() &&
           (currentTimestamp - symbolMeasurements.front().ts >
            MEASUREMENT_WINDOW_MS)) {
        symbolMeasurements.pop_front();
    }
}

std::vector<measurement_t> Measurement::getRecentMeasurements(
    const std::string& symbol, const int windowMs, long timestamp) {
    std::vector<measurement_t> result;

    const std::deque<measurement_t>& measurements = latestMeasurements[symbol];
    result.assign(measurements.begin(), measurements.end());

    return result;
}

void Measurement::storeMeasurement(const measurement_t& m) {
    // Store in memory first
    std::lock_guard<std::mutex> lock(measurementsMutex);
    latestMeasurements[m.instId].push_back(m);
    measurementsMutex.unlock();

    // Write to file
    std::string filename = "data/measurement.txt";

    // open the file for writing
    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    // write to the text file
    fprintf(fp, "%s %.6f %.6f %ld\n", m.instId.c_str(), m.px, m.sz, m.ts);

    fclose(fp);
};
