#pragma once

#include <chrono>
#include <string>

#include "allocator.h"

struct BenchmarkResult {
    std::string allocatorName;
    double avgAllocTimeNs;
    double avgFreeTimeNs;
    double utilizationFactor;
    size_t successfulAllocs;
    size_t failedAllocs;
};

class Benchmark {
public:
    static BenchmarkResult runBenchmark(Allocator* allocator, 
                                         size_t numOperations,
                                         size_t minSize, 
                                         size_t maxSize);
    
    static void comparePrint(const BenchmarkResult& r1, const BenchmarkResult& r2);
};

