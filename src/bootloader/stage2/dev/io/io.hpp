#pragma once
#include "dev/driver.hpp"
#include <stdint.h>
#include <stddef.h>

class BlockIODevice : public Driver {
public:
    virtual bool read_sectors(uint64_t lba, size_t count, void* buffer) = 0;
    virtual bool write_sectors(uint64_t lba, size_t count, const void* buffer) { return false; }
    virtual size_t sector_size() const { return 512; }
    virtual uint64_t total_sectors() const = 0;
};

class CharIODevice : public Driver {
public:
    virtual bool can_read() const { return false; }
    virtual uint8_t read_byte() = 0;
    virtual void write_byte(uint8_t byte) = 0;
};
