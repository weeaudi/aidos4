#include "dev/acpi/acpi.hpp"

ACPI::ACPI(uint32_t* RSDP){
    parseRSDP(RSDP);
}