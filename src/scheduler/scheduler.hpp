#pragma once

#include <pthread.h>

#include <atomic>
#include <string>
#include <vector>

// A simple scheduler that runs a task every minute
class Scheduler {
   private:
    pthread_t threadScheduler, threadAverage, threadPearson;
    std::atomic<bool> running;
    std::vector<std::string> SYMBOLS;

   public:
    Scheduler(std::vector<std::string> SYMBOLS);
    ~Scheduler();

    void start();
    void run();
    void stop();
};
