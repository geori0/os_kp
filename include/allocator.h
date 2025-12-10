#pragma once

#include <cstddef>

struct Allocator {
    virtual ~Allocator() = default;
    virtual void* alloc(size_t size) = 0;
    virtual void free(void* ptr) = 0;
    virtual const char* name() const = 0;
    virtual size_t getUsedMemory() const = 0;
    virtual size_t getTotalMemory() const = 0;
};

