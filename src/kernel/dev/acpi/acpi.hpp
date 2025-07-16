#include <stdint.h>

/**
 * @file acpi.hpp
 * @brief Basic ACPI structures and parser.
 */

/**
 * @brief Root System Description Pointer structure.
 */
struct AcpiRsdp {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;

    // ACPI 2.0+
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
};

class ACPI {
public:
    /** Construct an ACPI parser using the given RSDP address. */
    ACPI(uint32_t* RSDP);

private:
    void parseRSDP(uint32_t* RSDP);
    AcpiRsdp _rsdp;

};