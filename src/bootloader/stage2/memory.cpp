#include <stddef.h>
#include <stdint.h>
#include "memory.hpp"

static uint8_t* heap_base = &__heap_start;
static size_t heap_size = &__heap_end - &__heap_start;
static size_t heap_offset = 0;

void* kmalloc(size_t size, size_t align) {
    size_t current = reinterpret_cast<size_t>(heap_base) + heap_offset;
    size_t aligned = (current + align - 1) & ~(align - 1);
    size_t next_offset = aligned - reinterpret_cast<size_t>(heap_base) + size;

    if (next_offset >= heap_size) {
        return nullptr; // Out of memory
    }

    heap_offset = next_offset;
    return reinterpret_cast<void*>(aligned);
}

void* operator new(size_t size) {
    return kmalloc(size);
}
void* operator new[](size_t size) {
    return kmalloc(size);
}
void operator delete(void* ptr) noexcept {}
void operator delete[](void* ptr) noexcept {}
void operator delete(void* ptr, size_t) noexcept {}
void operator delete[](void* ptr, size_t) noexcept {}