#include "scheduler.hpp"

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include <chrono>
#include <cmath>
#include <iostream>

#include "../measurement/measurement.hpp"
#include "../data_collector/data_collector.hpp"
#include "../pearson/pearson.hpp"
#include "../utils/cpu_stats.hpp"

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

    pthread_cond_init(&scheduler->averageCondition, nullptr);
    pthread_cond_init(&scheduler->pearsonCondition, nullptr);
    pthread_mutex_init(&scheduler->averageMutex, nullptr);
    pthread_mutex_init(&scheduler->pearsonMutex, nullptr);
    scheduler->averageWorkReady = false;
    scheduler->pearsonWorkReady = false;

    return scheduler;
}

void Scheduler::destroy(scheduler_t& scheduler) {
    stop(scheduler);

    pthread_cond_destroy(&scheduler.averageCondition);
    pthread_cond_destroy(&scheduler.pearsonCondition);
    pthread_mutex_destroy(&scheduler.averageMutex);
    pthread_mutex_destroy(&scheduler.pearsonMutex);
}

void Scheduler::start(scheduler_t& scheduler) {
    if (!scheduler.running) {
        scheduler.running = true;
        active_scheduler = &scheduler;

        pthread_create(&scheduler.threadAverage, nullptr,
                       DataCollector::workerThread, &scheduler);
        pthread_create(&scheduler.threadPearson, nullptr, Pearson::workerThread,
                       &scheduler);
        pthread_create(&scheduler.threadScheduler, nullptr,
                       schedulerThreadFunction, &scheduler);
    }
}

void Scheduler::stop(scheduler_t& scheduler) {
    if (scheduler.running) {
        scheduler.running = false;

        // Signal worker threads to wake up and exit
        pthread_mutex_lock(&scheduler.averageMutex);
        scheduler.averageWorkReady = true;
        pthread_cond_signal(&scheduler.averageCondition);
        pthread_mutex_unlock(&scheduler.averageMutex);

        pthread_mutex_lock(&scheduler.pearsonMutex);
        scheduler.pearsonWorkReady = true;
        pthread_cond_signal(&scheduler.pearsonCondition);
        pthread_mutex_unlock(&scheduler.pearsonMutex);

        pthread_join(scheduler.threadScheduler, nullptr);
        pthread_join(scheduler.threadAverage, nullptr);
        pthread_join(scheduler.threadPearson, nullptr);
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

        scheduler.currentTimestamp = nextMinuteTimestampInMs;

        Measurement::cleanupOldMeasurements(scheduler.currentTimestamp);
        DataCollector::cleanupOldAverages(scheduler.currentTimestamp);

        // Calculate and write CPU stats
        double cpuIdlePercentage = CpuStats::getCpuIdlePercentage();
        if (cpuIdlePercentage >= 0.0) {
            CpuStats::writeCpuStats(scheduler.currentTimestamp,
                                    cpuIdlePercentage);
        }

        // Signal worker threads to start working
        pthread_mutex_lock(&scheduler.averageMutex);
        scheduler.averageWorkReady = true;
        pthread_cond_signal(&scheduler.averageCondition);
        pthread_mutex_unlock(&scheduler.averageMutex);

        pthread_mutex_lock(&scheduler.pearsonMutex);
        scheduler.pearsonWorkReady = true;
        pthread_cond_signal(&scheduler.pearsonCondition);
        pthread_mutex_unlock(&scheduler.pearsonMutex);
    }
}
