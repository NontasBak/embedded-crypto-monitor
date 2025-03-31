#pragma once

#include <pthread.h>

#include <atomic>
#include <functional>

// A simple scheduler that runs a task every minute
class Scheduler {
   private:
    std::function<void()> minute_task;
    pthread_t thread;
    std::atomic<bool> running;

    // Thread function that waits for the next minute and runs the task
    static void* threadFunction(void* arg);
    void run();

   public:
    Scheduler();
    ~Scheduler();

    // Set the function to run every minute
    void setMinuteTask(std::function<void()> task);

    // Start the scheduler
    void start();

    // Stop the scheduler
    void stop();
};
