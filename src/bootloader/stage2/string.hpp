
#pragma once

#include <stdint.h>

/**
 * @file string.hpp
 * @brief Small string utility functions.
 */

/** Find the first occurrence of a character in a string. */
inline char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == static_cast<char>(c))
            return const_cast<char*>(str);
        ++str;
    }
    return (c == '\0') ? const_cast<char*>(str) : nullptr;
}

/** Get the length of a null-terminated string. */
inline uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (str[len] != '\0') {
        ++len;
    }
    return len;
}

/** Convert a character to uppercase if applicable. */
inline char toupper(char c) {
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}