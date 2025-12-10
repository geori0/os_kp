#pragma once

#include <cstdint>
#include <vector>

#include "allocator.h"

class McKusickKarelsAllocator : public Allocator {
public:
    McKusickKarelsAllocator(void* memory, size_t size);
    ~McKusickKarelsAllocator() override = default;

    void* alloc(size_t size) override;
    void free(void* ptr) override;
    const char* name() const override { return "McKusick-Karels Allocator"; }
    size_t getUsedMemory() const override { return usedMemory_; }
    size_t getTotalMemory() const override { return totalSize_; }

private:
    static constexpr size_t PAGE_SIZE = 4096;
    static constexpr size_t MIN_BUCKET_SIZE = 16;
    static constexpr size_t NUM_BUCKETS = 12; 
    static constexpr size_t LARGE_ALLOC_THRESHOLD = PAGE_SIZE;

    struct FreeBlock {
        FreeBlock* next;
    };

    struct PageDescriptor {
        size_t bucketIndex;
        size_t allocCount;
        PageDescriptor* next;
        PageDescriptor* prev;
    };

    struct LargeBlock {
        size_t size;
        LargeBlock* next;
        LargeBlock* prev;
        bool isFree;
    };

    void* basePtr_;
    size_t totalSize_;
    size_t usedMemory_;

    std::vector<FreeBlock*> buckets_;
    std::vector<PageDescriptor*> partialPages_;
    PageDescriptor* freePages_;
    LargeBlock* largeBlocks_;

    size_t pageCount_;
    PageDescriptor* pageDescriptors_;
    void* dataStart_;

    size_t sizeToBucket(size_t size) const;
    size_t bucketToSize(size_t bucket) const;
    void* allocateFromBucket(size_t bucket);
    void* allocateLarge(size_t size);
    PageDescriptor* allocatePage(size_t bucket);
    void freeToBucket(void* ptr, PageDescriptor* page);
    void freeLarge(LargeBlock* block);
    PageDescriptor* getPageDescriptor(void* ptr) const;
};

McKusickKarelsAllocator* createMcKusickKarelsAllocator(void* realMemory, size_t memorySize);

