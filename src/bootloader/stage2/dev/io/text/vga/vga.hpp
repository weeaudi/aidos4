#pragma once

#include "dev/io/text/textdriver.hpp"
#include <stdint.h>

/**
 * @file vga.hpp
 * @brief Driver for the legacy VGA text console.
 */

/**
 * @brief Text output driver that writes to the VGA text buffer.
 */
class VGADriver : public TextOutDriver {
public:
    /** Initialize the VGA hardware and prepare for output. */
    bool initialize();

    /** Shutdown the VGA driver. */
    void shutdown();

    /** Output a single character to the screen. */
    void putchar(char c);

    /** Virtual destructor. */
    virtual ~VGADriver() {}

private:
    /** Enable the hardware cursor. */
    void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);

    /** Move the hardware cursor to the specified position. */
    void update_cursor(uint8_t x, uint8_t y);

    uint32_t x = 0; ///< Current cursor X position
    uint32_t y = 0; ///< Current cursor Y position
};