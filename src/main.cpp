#include <iostream>
#include <cstdlib>

#include "buddy_allocator.h"
#include "mckusick_karels_allocator.h"
#include "benchmark.h"

int main() {
    constexpr size_t MEMORY_SIZE = 64 * 1024 * 1024; 
    constexpr size_t NUM_OPERATIONS = 100000;
    constexpr size_t MIN_ALLOC_SIZE = 16;
    constexpr size_t MAX_ALLOC_SIZE = 4096;
    
    void* memory1 = std::aligned_alloc(4096, MEMORY_SIZE);
    void* memory2 = std::aligned_alloc(4096, MEMORY_SIZE);
    
    if (!memory1 || !memory2) {
        std::cerr << "Ошибка: не удалось выделить память\n";
        return 1;
    }
    
    std::cout << "Тест аллокатора с  " << (MEMORY_SIZE / 1024 / 1024) 
              << " MB пул мамяти\n";
    std::cout << "Операции: " << NUM_OPERATIONS << "\n";
    std::cout << "размеры аллокаций: " << MIN_ALLOC_SIZE << " - " << MAX_ALLOC_SIZE << " bytes\n";
    

    BuddyAllocator* buddy = createBuddyAllocator(memory1, MEMORY_SIZE);
    McKusickKarelsAllocator* mck = createMcKusickKarelsAllocator(memory2, MEMORY_SIZE);
    

    BenchmarkResult buddyResult = Benchmark::runBenchmark(buddy, NUM_OPERATIONS, 
                                                           MIN_ALLOC_SIZE, MAX_ALLOC_SIZE);
    BenchmarkResult mckResult = Benchmark::runBenchmark(mck, NUM_OPERATIONS,
                                                         MIN_ALLOC_SIZE, MAX_ALLOC_SIZE);
    
    Benchmark::comparePrint(buddyResult, mckResult);
    
    delete buddy;
    delete mck;
    std::free(memory1);
    std::free(memory2);
    
    return 0;
}
