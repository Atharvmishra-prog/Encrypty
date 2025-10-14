#pragma once
#include <iostream>
#include <chrono>

inline void print_progress(size_t processed, size_t total, std::chrono::steady_clock::time_point start_time, bool chunk) {
    int barWidth = 50;
    float progress = (float)processed / total;
    int pos = barWidth * progress;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
    std::cout << "\r[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% ("
              << processed << "/" << total << ((chunk)?" bytes, ":"files ")
              << elapsed << "s elapsed)" << std::flush;
}