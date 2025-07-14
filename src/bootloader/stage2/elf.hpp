#include <stdint.h>
#include "dev/io/fs/fs.hpp"

uint64_t readElf(FileSystem* fs, File* file, void* data);