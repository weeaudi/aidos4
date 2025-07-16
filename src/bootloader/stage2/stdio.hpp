#pragma once

#include "dev/io/text/textdriver.hpp"

/**
 * @file stdio.hpp
 * @brief Very small stdio-like helper functions.
 */

namespace stdio {

/** Output a string. */
void print(TextOutDriver& dev, const char* str);

/** Formatted print similar to printf. */
void printf(TextOutDriver& dev, const char* fmt, ...);

}
