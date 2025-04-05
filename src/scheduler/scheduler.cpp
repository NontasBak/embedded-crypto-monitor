#include "scheduler.hpp"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <iostream>

#include "../moving_average/moving_average.hpp"
#include "../pearson/pearson.hpp"

// Store a reference to the active scheduler
static scheduler_t* active_scheduler = nullptr;

void* schedulerThreadFunction(void* args) {
    scheduler_t* scheduler = (scheduler_t*)args;
    Scheduler::run(*scheduler);
    return nullptr;
}

scheduler_t* Scheduler::create(std::vector<std::string> SYMBOLS) {
    scheduler_t* scheduler = new scheduler_t();
    scheduler->SYMBOLS = SYMBOLS;
    scheduler->running = false;
    return scheduler;
}

void Scheduler::destroy(scheduler_t& scheduler) { stop(scheduler); }

void Scheduler::start(scheduler_t& scheduler) {
    if (!scheduler.running) {
        scheduler.running = true;
        active_scheduler = &scheduler;

        // Create the scheduler thread
        pthread_create(&scheduler.threadScheduler, nullptr,
                       schedulerThreadFunction, &scheduler);
    }
}

void Scheduler::stop(scheduler_t& scheduler) {
    if (scheduler.running) {
        scheduler.running = false;
        void* result;
        if (pthread_join(scheduler.threadScheduler, &result) != 0) {
            std::cerr << "Failed to join pthread" << std::endl;
        }
    }
}

void Scheduler::run(scheduler_t& scheduler) {
    while (scheduler.running) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now.time_since_epoch())
                             .count();
        long nextMinuteTimestampInMs = ((timestamp / 60000) + 1) * 60000;
        long msToWait = nextMinuteTimestampInMs - timestamp;

        std::cout << "Sleeping for " << msToWait / 1000 << " seconds"
                  << std::endl;

        usleep(msToWait * 1000);

        if (!scheduler.running) {
            return;
        }

        calculateAverageArgs argsAverage = {scheduler.SYMBOLS,
                                            nextMinuteTimestampInMs};
        calculatePearsonArgs argsPearson = {scheduler.SYMBOLS,
                                            nextMinuteTimestampInMs};

        // Create moving average and Pearson calculation threads
        pthread_create(&scheduler.threadAverage, nullptr,
                       MovingAverage::calculateAverage, (void*)&argsAverage);
        pthread_create(&scheduler.threadPearson, nullptr,
                       Pearson::calculateAllPearson, (void*)&argsPearson);

        // Wait for the threads to finish
        pthread_join(scheduler.threadAverage, nullptr);
        pthread_join(scheduler.threadPearson, nullptr);
    }
}
