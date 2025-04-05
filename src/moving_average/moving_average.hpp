#pragma once

#include <pthread.h>

#include <string>
#include <vector>

struct calculateAverageArgs {
    std::vector<std::string> SYMBOLS;
    long timestampInMs;
};

typedef struct {
    std::string symbol;
    double average;
    long timestamp;
} average_t;

namespace MovingAverage {

extern int window;  // In milliseconds

void writeAverageToFile(std::string symbol, double average, long timestamp,
                        int delay);
void* calculateAverage(void* arg);
std::vector<average_t> readAveragesFromFile(long timestamp);

}  // namespace MovingAverage
