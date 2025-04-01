#pragma once

#include <pthread.h>

#include <string>
#include <vector>

struct calculateAverageArgs {
    std::vector<std::string> SYMBOLS;
    long timestampInMs;
};

class MovingAverage {
   private:
    static int window;
    // static int average;
    // pthread_t thread;
    // double average;
   public:
    MovingAverage();
    ~MovingAverage();

    static void writeAverageToFile(std::string symbol, double average,
                                   long timestamp, int delay);
    static void* calculateAverage(void* arg);
};
