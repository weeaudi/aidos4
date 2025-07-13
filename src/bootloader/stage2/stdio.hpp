#pragma once

#include "dev/io/text/textdriver.hpp"

namespace stdio {

void print(TextOutDriver& dev, const char* str);
void printf(TextOutDriver& dev, const char* fmt, ...);

}
