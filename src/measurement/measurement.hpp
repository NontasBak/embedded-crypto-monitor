#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

typedef struct {
    std::string instId;  // Instrument ID (e.g., "BTC-USDT")
    double px;           // Price
    double sz;           // Size/volume
    long ts;             // Timestamp
} measurement_t;

namespace Measurement {

// In-memory storage for measurements (last 15 minutes)
extern std::map<std::string, std::deque<measurement_t>> latestMeasurements;
extern pthread_mutex_t measurementsMutex;
extern const long MEASUREMENT_WINDOW_MS;  // 15 minutes in milliseconds

measurement_t create(const std::string instId, double px, double sz, long ts);
void displayMeasurement(const measurement_t& m);
std::vector<measurement_t> getRecentMeasurements(const std::string& symbol,
                                                 const long windowMs,
                                                 long timestamp);
void storeMeasurement(const measurement_t& m);
void cleanupOldMeasurements(long currentTimestamp);

}  // namespace Measurement
