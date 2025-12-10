#include "benchmark.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <algorithm>

BenchmarkResult Benchmark::runBenchmark(Allocator* allocator,
                                         size_t numOperations,
                                         size_t minSize,
                                         size_t maxSize) {
    BenchmarkResult result;
    result.allocatorName = allocator->name();
    result.successfulAllocs = 0;
    result.failedAllocs = 0;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> sizeDist(minSize, maxSize);
    
    std::vector<void*> allocations;
    allocations.reserve(numOperations);
    
    auto allocStart = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < numOperations; i++) {
        size_t size = sizeDist(gen);
        void* ptr = allocator->alloc(size);
        if (ptr) {
            allocations.push_back(ptr);
            result.successfulAllocs++;
        } else {
            result.failedAllocs++;
        }
    }
    auto allocEnd = std::chrono::high_resolution_clock::now();
    
    result.utilizationFactor = static_cast<double>(allocator->getUsedMemory()) /
                               allocator->getTotalMemory();
    
    std::shuffle(allocations.begin(), allocations.end(), gen);
    
    auto freeStart = std::chrono::high_resolution_clock::now();
    for (void* ptr : allocations) {
        allocator->free(ptr);
    }
    auto freeEnd = std::chrono::high_resolution_clock::now();
    
    auto allocDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(allocEnd - allocStart);
    auto freeDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(freeEnd - freeStart);
    
    result.avgAllocTimeNs = static_cast<double>(allocDuration.count()) / numOperations;
    result.avgFreeTimeNs = static_cast<double>(freeDuration.count()) / 
                           (allocations.empty() ? 1 : allocations.size());
    
    return result;
}

void Benchmark::comparePrint(const BenchmarkResult& r1, const BenchmarkResult& r2) {
    std::cout << std::fixed << std::setprecision(2);
std::cout << "\n=============== РЕЗУЛЬТАТЫ СРАВНЕНИЯ ===============\n\n";

std::cout << std::fixed << std::setprecision(2);

std::cout << std::left << std::setw(42) << "Метрика" << " | " 
          << std::right << std::setw(20) << r1.allocatorName << " | " 
          << std::setw(20) << r2.allocatorName << "\n";

std::cout << std::string(81, '-') << "\n";


std::cout << std::left << std::setw(53) << "Ср. время выделения (нс)" << " | "
          << std::right << std::setw(20) << r1.avgAllocTimeNs << " | "
          << std::setw(20) << r2.avgAllocTimeNs << "\n";


std::cout << std::left << std::setw(56) << "Ср. время освобождения (нс)" << " | "
          << std::right << std::setw(20) << r1.avgFreeTimeNs << " | "
          << std::setw(20) << r2.avgFreeTimeNs << "\n";

std::cout << std::left << std::setw(54) << "Фактор использования" << " | "
          << std::right << std::setw(19) << (r1.utilizationFactor * 100) << "%" << " | "
          << std::setw(19) << (r2.utilizationFactor * 100) << "%\n";

std::cout << std::left << std::setw(52) << "Успешных выделений" << " | "
          << std::right << std::setw(20) << r1.successfulAllocs << " | "
          << std::setw(20) << r2.successfulAllocs << "\n";


std::cout << std::left << std::setw(53) << "Неудачных выделений" << " | "
          << std::right << std::setw(20) << r1.failedAllocs << " | "
          << std::setw(20) << r2.failedAllocs << "\n";

std::cout << std::string(81, '=') << "\n";

}
