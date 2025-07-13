#pragma once

#include "dev/io/text/textdriver.hpp"
#include <stdint.h>

class VGADriver : public TextOutDriver {
public:
    bool initialize();

    void shutdown();

    void putchar(char c);

    ~VGADriver() {}

private:
    void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
    void update_cursor(uint8_t x, uint8_t y);
    uint32_t x = 0;
    uint32_t y = 0;
};