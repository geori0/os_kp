#pragma once

#include <cstdint>
#include <vector>

#include "allocator.h"

class BuddyAllocator : public Allocator {
public:
    BuddyAllocator(void* memory, size_t size);
    ~BuddyAllocator() override = default;

    void* alloc(size_t size) override;
    void free(void* ptr) override;
    const char* name() const override { return "Buddy Allocator"; }
    size_t getUsedMemory() const override { return usedMemory_; }
    size_t getTotalMemory() const override { return totalSize_; }

private:
    static constexpr size_t MIN_BLOCK_SIZE = 32;
    static constexpr size_t MAX_LEVELS = 32;

    struct Block {
        Block* next;
        Block* prev;
        size_t level;
        bool isFree;
    };

    void* basePtr_;
    size_t totalSize_;
    size_t usedMemory_;
    size_t maxLevel_;
    std::vector<Block*> freeLists_;

    size_t sizeToLevel(size_t size) const;
    size_t levelToSize(size_t level) const;
    Block* getBuddy(Block* block) const;
    void removeFromFreeList(Block* block);
    void addToFreeList(Block* block);
    void splitBlock(Block* block, size_t targetLevel);
    void mergeBlock(Block* block);
};

BuddyAllocator* createBuddyAllocator(void* realMemory, size_t memorySize);

