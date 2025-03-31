#pragma once

#include <string>

class Measurement {
   private:
    std::string instId;  // Instrument ID (e.g., "BTC-USDT")
    double px;           // Price
    double sz;           // Size/volume
    long ts;             // Timestamp

   public:
    Measurement(const std::string instId, double px, double sz, long ts);
    ~Measurement();

    void displayMeasurement();
    void writeMeasurementToFile();
};
