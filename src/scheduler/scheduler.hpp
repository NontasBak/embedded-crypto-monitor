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
} scheduler_t;

namespace Scheduler {
    scheduler_t* create(std::vector<std::string> SYMBOLS);
    void destroy(scheduler_t& scheduler);
    void start(scheduler_t& scheduler);
    void run(scheduler_t& scheduler);
    void stop(scheduler_t& scheduler);
}
