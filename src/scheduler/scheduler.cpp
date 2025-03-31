#include "scheduler.hpp"

#include <unistd.h>

#include <chrono>
#include <iostream>

Scheduler::Scheduler() : running(false) {}

Scheduler::~Scheduler() { stop(); }

void Scheduler::setMinuteTask(std::function<void()> task) {
    minute_task = task;
}

void* Scheduler::threadFunction(void* arg) {
    Scheduler* scheduler = static_cast<Scheduler*>(arg);
    scheduler->run();
    return nullptr;
}

void Scheduler::start() {
    if (!running) {
        running = true;
        if (pthread_create(&thread, nullptr, &Scheduler::threadFunction,
                           this) != 0) {
            running = false;
            std::cerr << "Failed to create pthread" << std::endl;
        }
    }
}

void Scheduler::stop() {
    if (running) {
        running = false;
        void* result;
        if (pthread_join(thread, &result) != 0) {
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
        if (seconds_to_next_minute == 60) {
            seconds_to_next_minute = 0;
        }

        for (int i = 0; i < seconds_to_next_minute && running; i++) {
            sleep(1);
        }

        // Run the task if we're still running
        if (running && minute_task) {
            minute_task();
        }
    }
}
