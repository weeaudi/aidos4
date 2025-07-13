#pragma once
#include <stdint.h>

inline int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* a = static_cast<const unsigned char*>(ptr1);
    const unsigned char* b = static_cast<const unsigned char*>(ptr2);

    for (size_t i = 0; i < num; ++i) {
        if (a[i] != b[i])
            return (a[i] < b[i]) ? -1 : 1;
    }
    return 0;
}

inline void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = static_cast<unsigned char*>(dest);
    const unsigned char* s = static_cast<const unsigned char*>(src);

    for (size_t i = 0; i < num; ++i)
        d[i] = s[i];

    return dest;
}

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

void* kmalloc(size_t size, size_t align = 8);
void* operator new(size_t size);
void* operator new[](size_t size);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;