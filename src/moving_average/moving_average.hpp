#pragma once

#include <pthread.h>
#include <string>
#include <vector>

struct calculateAverageArgs {
    std::vector<std::string> SYMBOLS;
    long timestampInMs;
};

namespace MovingAverage {
    extern int window;  // In milliseconds

    void writeAverageToFile(std::string symbol, double average,
                            long timestamp, int delay);
    void* calculateAverage(void* arg);
}
