#pragma once
#include "../io.hpp"

class ATADrive : public BlockIODevice {
public:

    bool initialize() override;
    void shutdown() override;

    bool read_sectors(uint64_t lba, size_t count, void* buffer) override;
    bool write_sectors(uint64_t lba, size_t count, const void* buffer) override;

    size_t sector_size() const override;
    uint64_t total_sectors() const override;

    ~ATADrive() {}

private:
    uint16_t _io_base = 0;     // Set by initialize()
    bool _is_master = true;
    uint64_t _total_sectors = 0;
};
