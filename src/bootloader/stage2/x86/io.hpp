#pragma once

#include <stdint.h>

/**
 * @file io.hpp
 * @brief Low level I/O port access helpers.
 */

/** Write a byte to an I/O port. */
inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/** Read a byte from an I/O port. */
inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0"
                  : "=a"(value)
                  : "Nd"(port));
    return value;
}

/** Read a word from an I/O port. */
inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0"
                  : "=a"(ret)
                  : "Nd"(port));
    return ret;
}

/** Write a word to an I/O port. */
inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1"
                  :
                  : "a"(val), "Nd"(port));
}

