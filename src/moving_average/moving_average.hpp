#pragma once

#include <pthread.h>

class MovingAverage {
   private:
    // static int window;
    // static int average;
    // pthread_t thread;
    // double average;

    void writeAverageToFile(double average);

   public:
    MovingAverage();
    ~MovingAverage();

    static void* calculateAverage(void* arg);
};
