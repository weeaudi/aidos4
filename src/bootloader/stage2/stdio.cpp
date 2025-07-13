#include "stdio.hpp"

#include <stdarg.h>

namespace stdio {

void print(TextOutDriver& dev, const char* str) {
    while (*str) {
        dev.putchar(*str++);
    }
}

void printf(TextOutDriver& dev, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%' && *(fmt + 1)) {
            fmt++;
            switch (*fmt) {
                case 's': {
                    const char* s = va_arg(args, const char*);
                    print(dev, s);
                    break;
                }
                case 'd': {
                    int val = va_arg(args, int);
                    char buf[16];
                    // Simple itoa
                    char* p = buf + 15;
                    *p = '\0';
                    bool neg = val < 0;
                    unsigned int uval = neg ? -val : val;
                    do {
                        *--p = '0' + (uval % 10);
                        uval /= 10;
                    } while (uval);
                    if (neg) *--p = '-';
                    print(dev, p);
                    break;
                }
                case 'x': {
                    unsigned int val = va_arg(args, unsigned int);
                    char buf[16];
                    char* p = buf + 15;
                    *p = '\0';
                    do {
                        *--p = "0123456789ABCDEF"[val % 16];
                        val /= 16;
                    } while (val);
                    print(dev, p);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    dev.putchar(c);
                    break;
                }
                default:
                    dev.putchar('%');
                    dev.putchar(*fmt);
            }
        } else {
            dev.putchar(*fmt);
        }
        fmt++;
    }

    va_end(args);
}

} // namespace stdio
