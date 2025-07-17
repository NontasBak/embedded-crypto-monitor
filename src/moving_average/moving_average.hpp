#pragma once

#include <pthread.h>

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "../measurement/measurement.hpp"

struct calculateAverageArgs {
    std::vector<std::string> SYMBOLS;
    long timestampInMs;
};

typedef struct {
    std::string symbol;
    double average;
    long timestamp;
} average_t;

typedef struct {
    std::string symbol;
    double macd;
    long timestamp;
} macd_t;

typedef struct {
    std::string symbol;
    double signal;
    long timestamp;
} signal_t;

typedef struct {
    std::string symbol;
    double distance;
    long timestamp;
} distance_t;

typedef struct {
    std::vector<double> values;
    std::vector<long> timestamps;
} value_t;

namespace MovingAverage {

extern const long MA_WINDOW;
extern const long EMA_WINDOW;
extern const long SIGNAL_WINDOW;
extern const long AVERAGE_HISTORY_MS;
extern const long SHORT_TERM_EMA_WINDOW;
extern const long LONG_TERM_EMA_WINDOW;
extern std::map<std::string, std::deque<average_t>> latestAverages;
extern std::map<std::string, std::deque<average_t>> latestExponentialAverages;
extern std::map<std::string, std::deque<average_t>>
    latestShortTermEMA;  // 15 minutes
extern std::map<std::string, std::deque<average_t>>
    latestLongTermEMA;  // 30 minutes
extern std::map<std::string, std::deque<macd_t>> latestMACD;
extern std::map<std::string, std::deque<signal_t>> latestSignal;
extern std::map<std::string, std::deque<distance_t>> latestDistance;
extern pthread_mutex_t averagesMutex;

void storeAverage(std::string symbol, double average, long timestamp, int delay,
                  std::string type);
void cleanupOldAverages(long currentTimestamp);
void cleanupOldData(long currentTimestamp);
void* calculateAverage(std::vector<std::string> symbols, long currentTimestamp);
void* calculateAllExponentialAverages(std::vector<std::string> symbols,
                                      long currentTimestamp);
double calculateExponentialAverage(std::vector<measurement_t> measurements,
                                   const double previousEMA, long window);
void* calculateMACD(std::vector<std::string> symbols, long currentTimestamp);
void* calculateSignal(std::vector<std::string> symbols, long currentTimestamp,
                      long window);
void* calculateDistance(std::vector<std::string> symbols,
                        long currentTimestamp);
void* workerThread(void* arg);
value_t getRecentAverages(const std::string& symbol, long timestamp,
                          size_t window = 0);
value_t getRecentEMA(const std::string& symbol, long timestamp, size_t window,
                     std::string type);
value_t getRecentMACD(const std::string& symbol, long timestamp,
                      size_t window = 0);
value_t getRecentSignal(const std::string& symbol, long timestamp,
                        size_t window = 0);
value_t getRecentDistance(const std::string& symbol, long timestamp,
                          size_t window = 0);

}  // namespace MovingAverage
