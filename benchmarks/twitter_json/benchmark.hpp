
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <vector>
#include <format>
#include <filesystem>


namespace fs = std::filesystem;




template<typename Func>
double benchmark(const std::string& label, int iterations, Func&& func) {
    using clock = std::chrono::steady_clock;
    
    // Warm-up
    for (int i = 0; i < 3; ++i) {
        func();
    }
    
    auto start = clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = clock::now();
    
    auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double avg_us = static_cast<double>(total_us) / iterations;
    
    std::cout << std::format("{:<70} {:>8.2f} µs/iter  ({} iterations)\n",
                            label, avg_us, iterations);
    
    return avg_us;
}


