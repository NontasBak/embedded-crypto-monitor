#pragma once

#include <string>
#include <vector>

typedef struct {
    std::string instId;  // Instrument ID (e.g., "BTC-USDT")
    double px;           // Price
    double sz;           // Size/volume
    long ts;             // Timestamp
} measurement_t;

namespace Measurement {
    measurement_t create(const std::string instId, double px, double sz, long ts);
    void displayMeasurement(const measurement_t& m);
    std::vector<measurement_t> readMeasurementsFromFile(const int window, long timestamp);
    void writeMeasurementToFile(const measurement_t& m);
}
