#include "pearson.hpp"

#include <cstdio>
#include <iostream>

void Pearson::writePearsonToFile(double average) {
    std::string filename = "data/pearson.txt";

    // open the file for writing
    FILE* fp = fopen(filename.c_str(), "a");
    if (fp == NULL) {
        std::cout << "Error opening the file " << filename << std::endl;
        return;
    }

    // write to the text file
    fprintf(fp, "%.6f\n", average);
    fclose(fp);
}

void* Pearson::calculatePearson(void* arg) { 
    // Implementation will go here in future
    return nullptr; 
}
