#pragma once

#include <pthread.h>

#include <string>
#include <vector>

struct calculatePearsonArgs {
    std::vector<std::string> SYMBOLS;
    long timestampInMs;
};

namespace Pearson {

void writePearsonToFile(std::string symbol1, std::string symbol2,
                        double pearson, long timestamp, long maxTimestamp,
                        int delay);
void* calculateAllPearson(void* arg);
double calculatePearson(const std::vector<double>& x,
                        const std::vector<double>& y);

}  // namespace Pearson
