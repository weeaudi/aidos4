#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @file memory.hpp
 * @brief Memory and paging helpers for the bootloader.
 */

/** Compare two memory regions. */
inline int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* a = static_cast<const unsigned char*>(ptr1);
    const unsigned char* b = static_cast<const unsigned char*>(ptr2);

    for (size_t i = 0; i < num; ++i) {
        if (a[i] != b[i])
            return (a[i] < b[i]) ? -1 : 1;
    }
    return 0;
}

/** Copy a block of memory. */
inline void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = static_cast<unsigned char*>(dest);
    const unsigned char* s = static_cast<const unsigned char*>(src);

    for (size_t i = 0; i < num; ++i)
        d[i] = s[i];

    return dest;
}

/** Fill a block of memory with a value. */
inline void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = static_cast<unsigned char*>(ptr);
    unsigned char v = static_cast<unsigned char>(value);

    for (size_t i = 0; i < num; ++i)
        p[i] = v;

    return ptr;
}

extern "C" {
    extern uint8_t __heap_start;
    extern uint8_t __heap_end;
}

/** Allocate memory from the bootloader heap. */
void* kmalloc(size_t size, size_t align = 8);
/// Placement new operators backed by kmalloc
void* operator new(size_t size);
void* operator new[](size_t size);
void* operator new(size_t size, std::align_val_t a);
void* operator new[](size_t size, std::align_val_t a);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;

/// Page table flags
constexpr uint64_t PAGE_PRESENT = 1ull << 0;
constexpr uint64_t PAGE_RW      = 1ull << 1;
constexpr uint64_t PAGE_USER    = 1ull << 2;
constexpr uint64_t PAGE_PSE     = 1ull << 7;  // For 2MiB pages

constexpr uint64_t PAGE_SIZE_4K = 0x1000;   ///< 4 KiB page size
constexpr uint64_t PAGE_SIZE_2M = 0x200000; ///< 2 MiB page size

using PageTable = uint64_t[512];

extern "C" void load_cr3(uint64_t); ///< Assembly helper to load CR3

void setup_paging();
PageTable* getPML4();
PageTable* get_or_create_table(PageTable* parent, uint16_t index);
void map_4k(uint64_t virt, uint64_t phys, uint64_t count);
void map_2m(uint64_t virt, uint64_t phys, uint64_t count);
