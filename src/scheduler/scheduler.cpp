#include "scheduler.hpp"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <iostream>

#include "../moving_average/moving_average.hpp"
#include "../pearson/pearson.hpp"

void* schedulerThreadFunction(void* args) {
    Scheduler* scheduler = (Scheduler*)args;
    scheduler->run();
    return nullptr;
}

Scheduler::Scheduler(std::vector<std::string> SYMBOLS) : SYMBOLS(SYMBOLS) {
    running = false;
}

Scheduler::~Scheduler() { stop(); }

void Scheduler::start() {
    if (!running) {
        running = true;

        // Create the scheduler thread
        pthread_create(&threadScheduler, nullptr, schedulerThreadFunction,
                       this);
    }
}

void Scheduler::stop() {
    if (running) {
        running = false;
        void* result;
        if (pthread_join(threadScheduler, &result) != 0) {
            std::cerr << "Failed to join pthread" << std::endl;
        }
    }
}

void Scheduler::run() {
    while (running) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();
        long nextMinuteTimestampInMs = ((timestamp / 60000) + 1) * 60000;
        long msToWait = nextMinuteTimestampInMs - timestamp;

        std::cout << "Sleeping for " << msToWait / 1000 << " seconds"
                  << std::endl;

        usleep(msToWait * 1000);

        if (!running) {
            return;
        }

        calculateAverageArgs args = {SYMBOLS, nextMinuteTimestampInMs};

        // Create moving average and Pearson calculation threads
        pthread_create(&threadAverage, nullptr, MovingAverage::calculateAverage,
                       (void*)&args);
        // pthread_create(&threadPearson, nullptr, Pearson::calculatePearson,
        //                nullptr);

        // Wait for the threads to finish
        pthread_join(threadAverage, nullptr);
        // pthread_join(threadPearson, nullptr);
    }
}
