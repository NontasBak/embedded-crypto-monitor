#include "measurement.hpp"

#include <iostream>

Measurement::Measurement(const std::string instId, double px, double sz,
                         long ts)
    : instId(instId), px(px), sz(sz), ts(ts) {};

Measurement::~Measurement() {};

void Measurement::displayMeasurement() {
    std::cout << this->instId << " " << this->px << " " << this->sz << " "
              << this->ts << std::endl;
};

void Measurement::writeMeasurementToFile() {};
