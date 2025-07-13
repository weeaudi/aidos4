#pragma once

#include "dev/io/io.hpp"

class TextOutDriver : public CharIODevice {
public:
    // Output a single character
    virtual void putchar(char c) = 0;

    // Output a null-terminated string
    virtual void puts(const char* str) {
        while (*str) {
            putchar(*str++);
        }
    }

    virtual uint8_t read_byte() {
        return 0;
    }

    virtual void write_byte(uint8_t byte){
        putchar(byte);
    }

    ~TextOutDriver() {}
};