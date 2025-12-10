#include "mckusick_karels_allocator.h"

#include <algorithm>

McKusickKarelsAllocator::McKusickKarelsAllocator(void* memory, size_t size)
    : basePtr_(memory), totalSize_(size), usedMemory_(0),
      freePages_(nullptr), largeBlocks_(nullptr) {
    
    buckets_.resize(NUM_BUCKETS, nullptr);
    partialPages_.resize(NUM_BUCKETS, nullptr);

    size_t maxPages = size / PAGE_SIZE;
    size_t descriptorSpace = sizeof(PageDescriptor) * maxPages;
    descriptorSpace = (descriptorSpace + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
    
    pageDescriptors_ = static_cast<PageDescriptor*>(memory);
    dataStart_ = reinterpret_cast<uint8_t*>(memory) + descriptorSpace;
    pageCount_ = (size - descriptorSpace) / PAGE_SIZE;
    
    if (pageCount_ == 0) return; 

    for (size_t i = 0; i < pageCount_; i++) {
        pageDescriptors_[i].bucketIndex = SIZE_MAX;
        pageDescriptors_[i].allocCount = 0;
        pageDescriptors_[i].next = (i + 1 < pageCount_) ? &pageDescriptors_[i + 1] : nullptr;
        pageDescriptors_[i].prev = (i > 0) ? &pageDescriptors_[i - 1] : nullptr;
    }
    freePages_ = &pageDescriptors_[0];
}

size_t McKusickKarelsAllocator::sizeToBucket(size_t size) const {
    size = std::max(size, MIN_BUCKET_SIZE);
    size_t bucket = 0;
    size_t bucketSize = MIN_BUCKET_SIZE;
    while (bucketSize < size && bucket < NUM_BUCKETS - 1) {
        bucketSize *= 2;
        bucket++;
    }
    return bucket;
}

size_t McKusickKarelsAllocator::bucketToSize(size_t bucket) const {
    return MIN_BUCKET_SIZE << bucket;
}

McKusickKarelsAllocator::PageDescriptor* McKusickKarelsAllocator::getPageDescriptor(void* ptr) const {
    uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t dataAddr = reinterpret_cast<uintptr_t>(dataStart_);
    

    if (ptrAddr < dataAddr) return nullptr;
    
    uintptr_t offset = ptrAddr - dataAddr;
    size_t pageIndex = offset / PAGE_SIZE;
    
    if (pageIndex >= pageCount_) return nullptr;
    
    return &pageDescriptors_[pageIndex];
}

McKusickKarelsAllocator::PageDescriptor* McKusickKarelsAllocator::allocatePage(size_t bucket) {
    if (!freePages_) return nullptr;
    
    PageDescriptor* page = freePages_;
    freePages_ = page->next;
    if (freePages_) freePages_->prev = nullptr;
    
    page->bucketIndex = bucket;
    page->allocCount = 0;
    
    size_t blockSize = bucketToSize(bucket);
    size_t blocksPerPage = PAGE_SIZE / blockSize;
    size_t pageIndex = page - pageDescriptors_;
    
    if (pageIndex >= pageCount_) return nullptr; 
    
    uint8_t* pageStart = reinterpret_cast<uint8_t*>(dataStart_) + pageIndex * PAGE_SIZE;
    
    for (size_t i = 0; i < blocksPerPage; i++) {
        FreeBlock* block = reinterpret_cast<FreeBlock*>(pageStart + i * blockSize);
        block->next = buckets_[bucket];
        buckets_[bucket] = block;
    }
    
    page->next = partialPages_[bucket];
    page->prev = nullptr;
    if (partialPages_[bucket]) partialPages_[bucket]->prev = page;
    partialPages_[bucket] = page;
    
    return page;
}

void* McKusickKarelsAllocator::allocateFromBucket(size_t bucket) {
    if (bucket >= NUM_BUCKETS) return nullptr;
    
    if (!buckets_[bucket]) {
        if (!allocatePage(bucket)) return nullptr;
    }
    
    FreeBlock* block = buckets_[bucket];
    if (!block) return nullptr;
    
    buckets_[bucket] = block->next;
    
    PageDescriptor* page = getPageDescriptor(block);
    if (page) page->allocCount++;
    
    usedMemory_ += bucketToSize(bucket);
    return block;
}

void* McKusickKarelsAllocator::allocateLarge(size_t size) {
    size_t totalSize = size + sizeof(LargeBlock);
    size_t pagesNeeded = (totalSize + PAGE_SIZE - 1) / PAGE_SIZE;
    
    if (pagesNeeded > pageCount_) return nullptr;
    
    size_t consecutive = 0;
    PageDescriptor* firstPage = nullptr;
    
    for (PageDescriptor* page = freePages_; page; page = page->next) {
        size_t pageIdx = page - pageDescriptors_;
        if (pageIdx >= pageCount_) continue;
        
        if (firstPage) {
            size_t firstIdx = firstPage - pageDescriptors_;
            if (pageIdx == firstIdx + consecutive) {
                consecutive++;
            } else {
                firstPage = page;
                consecutive = 1;
            }
        } else {
            firstPage = page;
            consecutive = 1;
        }
        
        if (consecutive >= pagesNeeded) break;
    }
    
    if (consecutive < pagesNeeded || !firstPage) return nullptr;
    
    for (size_t i = 0; i < pagesNeeded; i++) {
        PageDescriptor* page = &firstPage[i];
        if (page->prev) page->prev->next = page->next;
        else freePages_ = page->next;
        if (page->next) page->next->prev = page->prev;
        page->bucketIndex = SIZE_MAX - 1;
        page->next = nullptr;
        page->prev = nullptr;
    }
    
    size_t pageIndex = firstPage - pageDescriptors_;
    LargeBlock* block = reinterpret_cast<LargeBlock*>(
        reinterpret_cast<uint8_t*>(dataStart_) + pageIndex * PAGE_SIZE);
    block->size = pagesNeeded * PAGE_SIZE;
    block->isFree = false;
    block->next = largeBlocks_;
    block->prev = nullptr;
    if (largeBlocks_) largeBlocks_->prev = block;
    largeBlocks_ = block;
    
    usedMemory_ += block->size;
    return reinterpret_cast<uint8_t*>(block) + sizeof(LargeBlock);
}

void* McKusickKarelsAllocator::alloc(size_t size) {
    if (size == 0) return nullptr;
    
    if (size <= LARGE_ALLOC_THRESHOLD / 2) {
        size_t bucket = sizeToBucket(size);
        return allocateFromBucket(bucket);
    }
    return allocateLarge(size);
}

void McKusickKarelsAllocator::freeToBucket(void* ptr, PageDescriptor* page) {
    if (!page || page->bucketIndex >= NUM_BUCKETS) return;
    
    size_t bucket = page->bucketIndex;
    FreeBlock* block = static_cast<FreeBlock*>(ptr);
    block->next = buckets_[bucket];
    buckets_[bucket] = block;
    
    if (page->allocCount > 0) page->allocCount--;
    usedMemory_ -= bucketToSize(bucket);
}

void McKusickKarelsAllocator::freeLarge(LargeBlock* block) {
    if (!block || block->size == 0) return;
    
    usedMemory_ -= block->size;
    
    if (block->prev) block->prev->next = block->next;
    else largeBlocks_ = block->next;
    if (block->next) block->next->prev = block->prev;
    
    uintptr_t blockAddr = reinterpret_cast<uintptr_t>(block);
    uintptr_t dataAddr = reinterpret_cast<uintptr_t>(dataStart_);
    
    if (blockAddr < dataAddr) return;
    
    size_t pageIndex = (blockAddr - dataAddr) / PAGE_SIZE;
    size_t numPages = block->size / PAGE_SIZE;
    
    if (pageIndex + numPages > pageCount_) return;
    
    for (size_t i = 0; i < numPages; i++) {
        PageDescriptor* page = &pageDescriptors_[pageIndex + i];
        page->bucketIndex = SIZE_MAX;
        page->allocCount = 0;
        page->next = freePages_;
        page->prev = nullptr;
        if (freePages_) freePages_->prev = page;
        freePages_ = page;
    }
}

void McKusickKarelsAllocator::free(void* ptr) {
    if (!ptr) return;
    
    uintptr_t ptrAddr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t dataAddr = reinterpret_cast<uintptr_t>(dataStart_);
    uintptr_t endAddr = dataAddr + pageCount_ * PAGE_SIZE;
    
    if (ptrAddr < dataAddr || ptrAddr >= endAddr) return;
    
    PageDescriptor* page = getPageDescriptor(ptr);
    if (!page) return;
    
    if (page->bucketIndex == SIZE_MAX - 1) {
        LargeBlock* block = reinterpret_cast<LargeBlock*>(
            reinterpret_cast<uint8_t*>(ptr) - sizeof(LargeBlock));
        freeLarge(block);
    } else if (page->bucketIndex < NUM_BUCKETS) {
        freeToBucket(ptr, page);
    }
}

McKusickKarelsAllocator* createMcKusickKarelsAllocator(void* realMemory, size_t memorySize) {
    return new McKusickKarelsAllocator(realMemory, memorySize);
}
