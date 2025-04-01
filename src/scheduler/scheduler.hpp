#pragma once

#include <pthread.h>

#include <atomic>

// A simple scheduler that runs a task every minute
class Scheduler {
   private:
    pthread_t threadScheduler, threadAverage, threadPearson;
    std::atomic<bool> running;

   public:
    Scheduler();
    ~Scheduler();

    void start();
    void run();
    void stop();
};
