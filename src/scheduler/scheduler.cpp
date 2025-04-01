#include "scheduler.hpp"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <iostream>

#include "../measurement/measurement.hpp"
#include "../moving_average/moving_average.hpp"
#include "../pearson/pearson.hpp"

void* schedulerThreadFunction(void* args) {
    Scheduler* scheduler = (Scheduler*)args;
    scheduler->run();
    return nullptr;
}

Scheduler::Scheduler() : running(false) {}

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
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::localtime(&now_time_t);

        int seconds_to_next_minute = 60 - now_tm->tm_sec;
        std::cout << "Seconds to next minute: " << seconds_to_next_minute
                  << std::endl;

        if (seconds_to_next_minute == 60) {
            seconds_to_next_minute = 0;
        }

        usleep(seconds_to_next_minute * 1000 * 1000);
        // pthread_create(&threadAverage, nullptr,
        //                &MovingAverage::calculateAverage, nullptr);
    }
}
