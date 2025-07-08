#include "cpu_stats.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "setup.hpp"

cpu_stats_t CpuStats::prev = {0};
cpu_stats_t CpuStats::current = {0};

double CpuStats::getCpuIdlePercentage() {
    std::ifstream file("/proc/stat");
    std::string line;

    if (!std::getline(file, line)) {
        std::cerr << "Failed to read /proc/stat" << std::endl;
        return -1.0;
    }

    std::istringstream iss(line);
    std::string cpu;

    iss >> cpu >> current.user >> current.nice >> current.system >>
        current.idle >> current.iowait >> current.irq >> current.softirq >>
        current.steal;

    current.total = current.user + current.nice + current.system +
                    current.idle + current.iowait + current.irq +
                    current.softirq + current.steal;

    if (prev.total == 0) {
        prev = current;
        return 0.0;
    }

    unsigned long long total_diff = current.total - prev.total;
    unsigned long long idle_diff =
        (current.idle + current.iowait) - (prev.idle + prev.iowait);

    double idle_percentage =
        (total_diff > 0) ? (100.0 * idle_diff / total_diff) : 0.0;

    prev = current;

    return idle_percentage;
}

void CpuStats::writeCpuStats(long timestamp, double idlePercentage) {
    std::string filePath = Setup::dataPath + "cpu_stats.txt";

    FILE* fp = fopen(filePath.c_str(), "a");
    if (fp == NULL) {
        std::cerr << "Failed to write CPU stats to file: " << filePath
                  << std::endl;
        return;
    }

    fprintf(fp, "%ld %.2f\n", timestamp, idlePercentage);
    fclose(fp);

    std::cout << "CPU idle percentage: " << idlePercentage << "%" << std::endl;
}
