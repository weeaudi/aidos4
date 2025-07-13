#include "vga.hpp"
#include "memory.hpp"
#include "x86/io.hpp"

#define SCREEN_ADDRESS (uint16_t*)0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define DEFAULT_ATTR 0x0F00

bool VGADriver::initialize(){
    uint16_t* screen = SCREEN_ADDRESS;
    for (uint16_t i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        screen[i] = DEFAULT_ATTR | ' ';
    }
    enable_cursor(14, 15);
    update_cursor(0, 0);
    return true;
}

void VGADriver::shutdown(){
    return;
}

void VGADriver::putchar(char c) {
    uint16_t* screen = SCREEN_ADDRESS;

    if (c == '\n') {
        y++;
        x = 0;
    } else {
        uint16_t cc = DEFAULT_ATTR | c;
        screen[y * SCREEN_WIDTH + x] = cc;
        x++;

        if (x >= SCREEN_WIDTH) {
            x = 0;
            y++;
        }
    }

    if (y >= SCREEN_HEIGHT) {
        for (int row = 1; row < SCREEN_HEIGHT; ++row) {
            for (int col = 0; col < SCREEN_WIDTH; ++col) {
                screen[(row - 1) * SCREEN_WIDTH + col] =
                    screen[row * SCREEN_WIDTH + col];
            }
        }

        for (int col = 0; col < SCREEN_WIDTH; ++col) {
            screen[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH + col] = DEFAULT_ATTR | ' ';
        }

        y = SCREEN_HEIGHT - 1;
    }

    update_cursor(x, y);
}

#include <stdint.h>

void VGADriver::enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void VGADriver::update_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * 80 + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));       // Low byte

    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF)); // High byte
}
