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
pthread_mutex_t Measurement::measurementsMutex;
const long Measurement::MEASUREMENT_WINDOW_MS =
    15 * 60 * 1000;  // 15 minutes

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

void Measurement::cleanupOldMeasurements(long currentTimestamp) {
    for (auto& pair : latestMeasurements) {
        std::deque<measurement_t>& symbolMeasurements = pair.second;
        while (!symbolMeasurements.empty() &&
               (currentTimestamp - symbolMeasurements.front().ts >
                MEASUREMENT_WINDOW_MS)) {
            symbolMeasurements.pop_front();
        }
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
    pthread_mutex_lock(&measurementsMutex);
    latestMeasurements[m.instId].push_back(m);
    pthread_mutex_unlock(&measurementsMutex);

    // Write to symbol-specific file
    std::string filename = "data/meas_" + m.instId + ".txt";

    // open the file for writing
    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch())
                         .count();
    auto delay = timestamp - m.ts;

    // write to the text file
    fprintf(fp, "%.6f %.6f %ld %ld\n", m.px, m.sz, m.ts, delay);
    fclose(fp);
}
