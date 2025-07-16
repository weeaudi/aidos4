
#pragma once
#include "dev/driver.hpp"
#include <stdint.h>
#include <stddef.h>

/**
 * @file io.hpp
 * @brief Base interfaces for generic I/O devices.
 */

/**
 * @brief Interface for block based I/O devices.
 */
class BlockIODevice : public Driver {
public:
    /**
     * @brief Read one or more sectors from the device.
     */
    virtual bool read_sectors(uint64_t lba, size_t count, void* buffer) = 0;

    /**
     * @brief Write one or more sectors to the device.
     *
     * The default implementation returns false, indicating that the
     * operation is unsupported.
     */
    virtual bool write_sectors(uint64_t lba, size_t count, const void* buffer) { return false; }

    /**
     * @brief Size of a single sector in bytes.
     */
    virtual size_t sector_size() const { return 512; }

    /**
     * @brief Total number of sectors on the device.
     */
    virtual uint64_t total_sectors() const = 0;
};

/**
 * @brief Interface for character based I/O devices.
 */
class CharIODevice : public Driver {
public:
    /** Check if a byte can be read without blocking. */
    virtual bool can_read() const { return false; }

    /** Read a single byte from the device. */
    virtual uint8_t read_byte() = 0;

    /** Write a single byte to the device. */
    virtual void write_byte(uint8_t byte) = 0;
};
