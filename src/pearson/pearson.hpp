#pragma once

#include <pthread.h>

namespace Pearson {
    void writePearsonToFile(double average);
    void* calculatePearson(void* arg);
}
