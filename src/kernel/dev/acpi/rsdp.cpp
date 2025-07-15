#include "dev/acpi/acpi.hpp"
#include "memory.hpp"

void ACPI::parseRSDP(uint32_t* RSDP) {
    map_4k((uint64_t)RSDP, (uint64_t)RSDP, 1);
    memcpy(&_rsdp, RSDP, sizeof(AcpiRsdp));
    unmap_4k((uint64_t)RSDP, 1);
}