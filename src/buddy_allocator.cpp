#include "buddy_allocator.h"

BuddyAllocator::BuddyAllocator(void* memory, size_t size)
    : basePtr_(memory), totalSize_(size), usedMemory_(0) {
    
    maxLevel_ = 0;
    size_t s = MIN_BLOCK_SIZE;
    while (s < size && maxLevel_ < MAX_LEVELS - 1) {
        s *= 2;
        maxLevel_++;
    }
    totalSize_ = levelToSize(maxLevel_);

    freeLists_.resize(maxLevel_ + 1, nullptr);

    Block* initialBlock = static_cast<Block*>(memory);
    initialBlock->next = nullptr;
    initialBlock->prev = nullptr;
    initialBlock->level = maxLevel_;
    initialBlock->isFree = true;

    freeLists_[maxLevel_] = initialBlock;
}

size_t BuddyAllocator::sizeToLevel(size_t size) const {
    size += sizeof(Block);
    size_t level = 0;
    size_t blockSize = MIN_BLOCK_SIZE;
    while (blockSize < size && level < maxLevel_) {
        blockSize *= 2;
        level++;
    }
    return level;
}

size_t BuddyAllocator::levelToSize(size_t level) const {
    return MIN_BLOCK_SIZE << level;
}

BuddyAllocator::Block* BuddyAllocator::getBuddy(Block* block) const {
    size_t blockSize = levelToSize(block->level);
    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(basePtr_);
    uintptr_t blockAddr = reinterpret_cast<uintptr_t>(block);
    uintptr_t offset = blockAddr - baseAddr;
    uintptr_t buddyOffset = offset ^ blockSize;
    
    if (buddyOffset >= totalSize_) return nullptr;
    if (buddyOffset + blockSize > totalSize_) return nullptr;
    
    return reinterpret_cast<Block*>(baseAddr + buddyOffset);
}

void BuddyAllocator::removeFromFreeList(Block* block) {
    if (!block) return;
    
    size_t level = block->level;
    if (level > maxLevel_) return;  
    
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        freeLists_[level] = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    block->next = nullptr;
    block->prev = nullptr;
}

void BuddyAllocator::addToFreeList(Block* block) {
    if (!block) return;
    
    size_t level = block->level;
    if (level > maxLevel_) return; 
    
    block->next = freeLists_[level];
    block->prev = nullptr;
    if (freeLists_[level]) {
        freeLists_[level]->prev = block;
    }
    freeLists_[level] = block;
    block->isFree = true;
}

void BuddyAllocator::splitBlock(Block* block, size_t targetLevel) {
    while (block->level > targetLevel) {
        removeFromFreeList(block);
        block->level--;
        
        size_t newSize = levelToSize(block->level);
        Block* buddy = reinterpret_cast<Block*>(
            reinterpret_cast<uint8_t*>(block) + newSize);
        
        uintptr_t buddyEnd = reinterpret_cast<uintptr_t>(buddy) + sizeof(Block);
        uintptr_t memEnd = reinterpret_cast<uintptr_t>(basePtr_) + totalSize_;
        if (buddyEnd > memEnd) {
            block->level++;
            addToFreeList(block);
            return;
        }
        
        buddy->level = block->level;
        buddy->isFree = true;
        buddy->next = nullptr;
        buddy->prev = nullptr;
        
        addToFreeList(block);
        addToFreeList(buddy);
    }
}

void BuddyAllocator::mergeBlock(Block* block) {
    while (block->level < maxLevel_) {
        Block* buddy = getBuddy(block);
        
        if (!buddy) break;
        
        uintptr_t buddyAddr = reinterpret_cast<uintptr_t>(buddy);
        uintptr_t baseAddr = reinterpret_cast<uintptr_t>(basePtr_);
        if (buddyAddr < baseAddr || buddyAddr >= baseAddr + totalSize_) break;
        
        if (!buddy->isFree) break;
        if (buddy->level != block->level) break;
        
        removeFromFreeList(buddy);
        removeFromFreeList(block);
        
        if (buddy < block) block = buddy;
        block->level++;
        
        addToFreeList(block);
    }
}

void* BuddyAllocator::alloc(size_t size) {
    if (size == 0) return nullptr;
    
    size_t level = sizeToLevel(size);
    if (level > maxLevel_) return nullptr;

    size_t searchLevel = level;
    while (searchLevel <= maxLevel_ && !freeLists_[searchLevel]) {
        searchLevel++;
    }
    
    if (searchLevel > maxLevel_) return nullptr;

    Block* block = freeLists_[searchLevel];
    if (!block) return nullptr; 
    
    if (searchLevel > level) {
        splitBlock(block, level);
        block = freeLists_[level];
        if (!block) return nullptr; 
    }
    
    removeFromFreeList(block);
    block->isFree = false;
    
    usedMemory_ += levelToSize(level);
    
    return reinterpret_cast<void*>(
        reinterpret_cast<uint8_t*>(block) + sizeof(Block));
}

void BuddyAllocator::free(void* ptr) {
    if (!ptr) return;
    
    uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(basePtr_);
    if (ptrAddr <= baseAddr || ptrAddr >= baseAddr + totalSize_) return;
    
    Block* block = reinterpret_cast<Block*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(Block));
    
    if (reinterpret_cast<uintptr_t>(block) < baseAddr) return;
    if (block->level > maxLevel_) return;
    if (block->isFree) return;
    
    usedMemory_ -= levelToSize(block->level);
    block->isFree = true;
    addToFreeList(block);
    mergeBlock(block);
}

BuddyAllocator* createBuddyAllocator(void* realMemory, size_t memorySize) {
    return new BuddyAllocator(realMemory, memorySize);
}
