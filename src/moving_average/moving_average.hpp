#pragma once

#include <pthread.h>

#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct calculateAverageArgs {
    std::vector<std::string> SYMBOLS;
    long timestampInMs;
};

typedef struct {
    std::string symbol;
    double average;
    long timestamp;
} average_t;

namespace MovingAverage {

extern int window;
extern const long AVERAGE_HISTORY_MS;  // 60 minutes history in milliseconds
extern std::map<std::string, std::deque<average_t>> latestAverages;
extern std::mutex averagesMutex;

void storeAverage(std::string symbol, double average, long timestamp,
                  int delay);
void cleanupOldAverages(const std::string& symbol, long currentTimestamp);
void* calculateAverage(void* arg);
void* workerThread(void* arg);
std::vector<double> getRecentAverages(const std::string& symbol, long timestamp,
                                      size_t window = 0);

}  // namespace MovingAverage
