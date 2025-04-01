#pragma once

#include <pthread.h>

class Pearson {
   private:
    // static int window;
    // static int average;
    // pthread_t thread;
    // double average;

    void writePearsonToFile(double average);

   public:
    Pearson();
    ~Pearson();

    static void* calculatePearson(void* arg);
};
