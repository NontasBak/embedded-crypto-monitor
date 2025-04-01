#include "measurement.hpp"

#include <cstddef>
#include <cstdio>
#include <iostream>

Measurement::Measurement(const std::string instId, double px, double sz,
                         long ts) {
    m.instId = instId;
    m.px = px;
    m.sz = sz;
    m.ts = ts;
};

Measurement::~Measurement() {};

void Measurement::displayMeasurement() {
    std::cout << this->m.instId << " " << this->m.px << " " << this->m.sz << " "
              << this->m.ts << std::endl;
};

std::vector<measurement> Measurement::readMeasurementsFromFile(
    const int windowMs, long timestamp) {
    FILE* fp = fopen("data/measurement.txt", "r");
    if (fp == NULL) {
        std::cerr << "Error opening file" << std::endl;
        throw std::runtime_error("Failed to open measurement file");
    }

    std::vector<measurement> measurements;
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
            measurement m;
            char instId[32];  // Adjust size as needed
            sscanf(buffer, "%s %lf %lf %ld", instId, &m.px, &m.sz, &m.ts);
            m.instId = std::string(instId);

            // Check if we're within the time window
            if (timestamp - m.ts > windowMs) {
                break;
            }

            measurements.push_back(m);
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

    return measurements;
}

void Measurement::writeMeasurementToFile() {
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
