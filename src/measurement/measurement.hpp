#pragma once

#include <string>
#include <vector>

typedef struct {
    std::string instId;  // Instrument ID (e.g., "BTC-USDT")
    double px;           // Price
    double sz;           // Size/volume
    long ts;             // Timestamp
} measurement;

class Measurement {
   private:
    measurement m;

   public:
    Measurement(const std::string instId, double px, double sz, long ts);
    ~Measurement();

    void displayMeasurement();
    static std::vector<measurement> readMeasurementsFromFile(const int window);
    void writeMeasurementToFile();
};
