#pragma once
#include "../io.hpp"

/**
 * @file ata.hpp
 * @brief ATA disk driver used during boot.
 */

/**
 * @brief Block device driver for ATA disks.
 */
class ATADrive : public BlockIODevice {
public:

    /** Initialise the drive. */
    bool initialize() override;
    /** Shutdown the drive. */
    void shutdown() override;

    /** Read sectors from disk. */
    bool read_sectors(uint64_t lba, size_t count, void* buffer) override;
    /** Write sectors to disk. */
    bool write_sectors(uint64_t lba, size_t count, const void* buffer) override;

    /** Size of a sector in bytes. */
    size_t sector_size() const override;
    /** Total sectors available on the disk. */
    uint64_t total_sectors() const override;

    /** Virtual destructor. */
    virtual ~ATADrive() {}

private:
    uint16_t _io_base = 0;     ///< I/O base address determined at init
    bool _is_master = true;    ///< true if device is master
    uint64_t _total_sectors = 0; ///< Cached total sector count
};
