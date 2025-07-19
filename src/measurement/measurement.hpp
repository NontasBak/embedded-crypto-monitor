#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

typedef struct {
    double px;  // Price
    double sz;  // Size/volume
    long ts;    // Timestamp
} measurement_t;

namespace Measurement {

// In-memory storage for measurements (last 15 minutes)
extern std::map<std::string, std::deque<measurement_t>> latestMeasurements;
extern pthread_mutex_t measurementsMutex;
extern const long MEASUREMENT_WINDOW_MS;  // 15 minutes in milliseconds

measurement_t create(double px, double sz, long ts);
void displayMeasurement(const measurement_t& m);
std::vector<measurement_t> getRecentMeasurements(const std::string& symbol,
                                                 const long windowMs,
                                                 long timestamp);
void storeMeasurement(const std::string& symbol, const measurement_t& m);
void cleanupOldMeasurements(long currentTimestamp);

}  // namespace Measurement
