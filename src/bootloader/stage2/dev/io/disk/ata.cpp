#include "ata.hpp"
#include "x86/io.hpp"

#define ATA_PRIMARY_IO_BASE 0x1F0
#define ATA_PRIMARY_CTRL_BASE 0x3F6

#define SECTOR_SIZE 512

constexpr int MAX_WAIT = 100000;

// IO Ports per channel
struct ATAChannel {
    uint16_t io_base;  // Base I/O port (e.g. 0x1F0 or 0x170)
    uint16_t ctrl_base; // Optional control base (not used here)
};

// List of ports to try: Primary and Secondary
static const ATAChannel channels[2] = {
    {0x1F0, 0x3F6},  // Primary
    {0x170, 0x376}   // Secondary
};

// Drive select values
static const uint8_t drives[2] = {
    0xA0, // Master
    0xB0  // Slave
};

bool ATADrive::initialize() {

    uint32_t sectors28;

    for (int ch = 0; ch < 2; ch++) {
        for (int drv = 0; drv < 2; drv++) {
            uint16_t io = channels[ch].io_base;
            uint8_t drive_select = drives[drv];

            // Select drive
            outb(io + 6, drive_select);
            
            // Short delay after select
            for (int i = 0; i < 4; i++) inb(io + 7);

            // Wait for BSY to clear
            int wait = 0;
            uint8_t status;
            do {
                status = inb(io + 7);  // Status register
                if (++wait > MAX_WAIT) goto next_drive;
            } while (status & 0x80); // BSY

            // Send IDENTIFY command
            outb(io + 7, 0xEC);

            // Wait for DRQ or ERR
            wait = 0;
            while (true) {
                status = inb(io + 7);
                if (status & 0x01) goto next_drive;  // ERR
                if (status & 0x08) break;  // DRQ
                if (++wait > MAX_WAIT) goto next_drive;
            }

            // Read IDENTIFY data
            uint16_t identify_data[256];
            for (int i = 0; i < 256; i++) {
                identify_data[i] = inw(io + 0);
            }

            // Get total sectors from words 60–61
            sectors28 = (identify_data[61] << 16) | identify_data[60];
            _total_sectors = sectors28;
            _io_base = io;
            _is_master = (drv == 0);
            return true;

        next_drive:
            continue;
        }
    }

    // No working drive found
    return false;
}

void ATADrive::shutdown(){}

bool ATADrive::read_sectors(uint64_t lba, size_t count, void* buffer) {
    if (count == 0 || buffer == nullptr || !_io_base) {
        return false;
    }

    // Only support 28-bit LBA
    if (lba > 0x0FFFFFFF) {
        return false;
    }

    constexpr int MAX_WAIT = 100000;
    uint8_t* buf = reinterpret_cast<uint8_t*>(buffer);

    for (size_t sector = 0; sector < count; ++sector) {
        uint32_t cur_lba = static_cast<uint32_t>(lba + sector);

        // Wait for BSY to clear
        int wait = 0;
        while (inb(_io_base + 7) & 0x80) {  // BSY
            if (++wait > MAX_WAIT) return false;
        }

        // Select drive + LBA high nibble + LBA mode
        uint8_t drive_head = (_is_master ? 0xE0 : 0xF0) | ((cur_lba >> 24) & 0x0F);
        outb(_io_base + 6, drive_head);

        // Set sector count to 1
        outb(_io_base + 2, 1);

        // Set LBA low/mid/high
        outb(_io_base + 3, static_cast<uint8_t>(cur_lba & 0xFF));         // LBA Low
        outb(_io_base + 4, static_cast<uint8_t>((cur_lba >> 8) & 0xFF));  // LBA Mid
        outb(_io_base + 5, static_cast<uint8_t>((cur_lba >> 16) & 0xFF)); // LBA High

        // Send READ SECTORS command
        outb(_io_base + 7, 0x20);

        // Wait for DRQ or ERR
        wait = 0;
        while (true) {
            uint8_t status = inb(_io_base + 7);
            if (status & 0x01) return false; // ERR
            if (status & 0x08) break;        // DRQ
            if (++wait > MAX_WAIT) return false;
        }

        // Read 256 words (512 bytes)
        for (int i = 0; i < 256; i++) {
            uint16_t data = inw(_io_base + 0);  // DATA register
            buf[sector * 512 + i * 2]     = data & 0xFF;
            buf[sector * 512 + i * 2 + 1] = (data >> 8) & 0xFF;
        }
    }

    return true;
}

bool ATADrive::write_sectors(uint64_t /*lba*/, size_t /*count*/, const void* /*buffer*/) {
    // Writing not supported in bootloader stage
    return false;
}

size_t ATADrive::sector_size() const {
    return SECTOR_SIZE;
}

uint64_t ATADrive::total_sectors() const {
    return _total_sectors;
}