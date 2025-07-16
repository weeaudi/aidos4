#pragma once

#include "dev/io/io.hpp"

/**
 * @file textdriver.hpp
 * @brief Base class for text output devices.
 */

/**
 * @brief Character output device capable of writing text.
 */
class TextOutDriver : public CharIODevice {
public:
    /** Output a single character. */
    virtual void putchar(char c) = 0;

    /**
     * @brief Output a null-terminated string.
     */
    virtual void puts(const char* str) {
        while (*str) {
            putchar(*str++);
        }
    }

    /**
     * @brief Read a byte from the device.
     *
     * Text output devices typically do not support reading. The default
     * implementation returns 0.
     */
    virtual uint8_t read_byte() {
        return 0;
    }

    /**
     * @brief Write a byte to the device.
     */
    virtual void write_byte(uint8_t byte){
        putchar(byte);
    }

    /** Virtual destructor. */
    virtual ~TextOutDriver() {}
};