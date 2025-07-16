#include <stdint.h>
#include "memory.hpp"
#include "dev/acpi/acpi.hpp"


#pragma pack(push, 1)         

struct MemoryRegion
{
    uint64_t Base;
    uint64_t Length;
    uint32_t type;
    uint32_t extra;
};


struct BootInfo
{
    MemoryRegion    mem_map[2048 / sizeof(MemoryRegion)];
    uint8_t         reserved[2048 - ((2048 / sizeof(MemoryRegion)) * sizeof(MemoryRegion))];  
    uint32_t        mem_map_count;  
    uint32_t        rsdp_addr;      
    uint32_t        ebda;           
    uint8_t         boot_drive;     
};

struct BootInfoExtended
{
    uint8_t memoryMapIndex;
    uint64_t* PML4Address;
    uint64_t kernelPhysicalBase;
};
#pragma pack(pop)

alignas(16) BootInfo bootInfoStatic;
alignas(16) BootInfoExtended bootInfoExtendedStatic;

extern "C" void main(BootInfo* bootInfo, BootInfoExtended* bootInfoExtended) {

    memcpy(&bootInfoStatic, bootInfo, sizeof(BootInfo));
    memcpy(&bootInfoExtendedStatic, bootInfoExtended, sizeof(BootInfoExtended));

    setup_paging(bootInfoExtendedStatic.PML4Address);

    ACPI acpi = ACPI((uint32_t*)bootInfoStatic.rsdp_addr);

    while(true){}
}