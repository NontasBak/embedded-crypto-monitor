#include "measurement.hpp"

#include <cstdio>
#include <iostream>

Measurement::Measurement(const std::string instId, double px, double sz,
                         long ts)
    : instId(instId), px(px), sz(sz), ts(ts) {};

Measurement::~Measurement() {};

void Measurement::displayMeasurement() {
    std::cout << this->instId << " " << this->px << " " << this->sz << " "
              << this->ts << std::endl;
};

void Measurement::writeMeasurementToFile() {
    std::string filename = "data/measurement.txt";

    // open the file for writing
    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    // write to the text file
    fprintf(fp, "%s %.6f %.6f %ld\n", instId.c_str(), px, sz, ts);

    fclose(fp);
};
