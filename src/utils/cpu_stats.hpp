#pragma once

struct cpu_stats_t {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long total;
};

namespace CpuStats {

extern cpu_stats_t prev;
extern cpu_stats_t current;

double getCpuIdlePercentage();
void writeCpuStats(long timestamp, double idlePercentage);

}  // namespace CpuStats
