#pragma once

#include <pthread.h>

#include <atomic>
#include <string>
#include <vector>

typedef struct {
    pthread_t threadScheduler;
    pthread_t threadAverage;
    pthread_t threadPearson;
    std::atomic<bool> running;
    std::vector<std::string> SYMBOLS;

    // Synchronization primitives
    pthread_cond_t averageCondition;
    pthread_cond_t pearsonCondition;
    pthread_mutex_t averageMutex;
    pthread_mutex_t pearsonMutex;
    bool averageWorkReady;
    bool pearsonWorkReady;
    long currentTimestamp;
} scheduler_t;

namespace Scheduler {

scheduler_t* create(std::vector<std::string> SYMBOLS);
void destroy(scheduler_t& scheduler);
void start(scheduler_t& scheduler);
void run(scheduler_t& scheduler);
void stop(scheduler_t& scheduler);

}  // namespace Scheduler
