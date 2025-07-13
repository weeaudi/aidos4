#include "dev/io/fs/fat/fat.hpp"

FileSystem* FAT12FileSystem::detect(BlockIODevice* disk)
{
    uint8_t sector[512];
    if (!disk->read_sectors(0, 1, sector)) return nullptr;
    if (sector[510] != 0x55 || sector[511] != 0xAA) return nullptr;   // boot sig

    /* Local BPB view (only what we need) */
    #pragma pack(push,1)
    struct BPB {
        uint8_t   jmpBoot[3];
        char      oem[8];
        uint16_t  bytesPerSector;      // 0x0B
        uint8_t   sectorsPerCluster;   // 0x0D
        uint16_t  reservedSectorCount; // 0x0E
        uint8_t   numFATs;             // 0x10
        uint16_t  rootEntryCount;      // 0x11
        uint16_t  totalSectors16;      // 0x13
        uint8_t   media;               // 0x15
        uint16_t  fatSize16;           // 0x16
        uint16_t  sectorsPerTrack;     // 0x18
        uint16_t  numHeads;            // 0x1A
        uint32_t  hiddenSectors;       // 0x1C
        uint32_t  totalSectors32;      // 0x20
    };
    #pragma pack(pop)

    const BPB* bpb = reinterpret_cast<const BPB*>(sector);

    auto le16 = [](uint16_t v){ return v; };
    auto le32 = [](uint32_t v){ return v; };

    uint16_t bps = le16(bpb->bytesPerSector);
    uint8_t  spc =  bpb->sectorsPerCluster;
    if (!bps || (bps & (bps-1)) || !spc || (spc & (spc-1))) return nullptr;

    uint32_t totSec = le16(bpb->totalSectors16);
    if (!totSec) totSec = le32(bpb->totalSectors32);

    uint32_t fatSz  = le16(bpb->fatSize16);
    if (!fatSz)      return nullptr;              // FAT12 always stores size here

    uint32_t rootDirSec =
        ((le16(bpb->rootEntryCount) * 32u) + (bps - 1)) / bps;

    uint32_t dataSec = totSec -
                       (le16(bpb->reservedSectorCount) +
                        bpb->numFATs * fatSz +
                        rootDirSec);

    uint32_t clusters = dataSec / spc;
    if (clusters >= 4085) return nullptr;         // not FAT12

    auto* fs = new FAT12FileSystem();
    fs->set_disk(disk);
    return fs;
}

// FileSystem* FAT16FileSystem::detect(BlockIODevice* disk)
// {
//     uint8_t sector[512];
//     if (!disk->read_sectors(0, 1, sector)) return nullptr;
//     if (sector[510] != 0x55 || sector[511] != 0xAA) return nullptr;

//     #pragma pack(push,1)
//     struct BPB {
//         uint8_t   jmpBoot[3];  char oem[8];
//         uint16_t  bytesPerSector;      uint8_t  sectorsPerCluster;
//         uint16_t  reservedSectorCount; uint8_t  numFATs;
//         uint16_t  rootEntryCount;      uint16_t totalSectors16;
//         uint8_t   media;               uint16_t fatSize16;
//         uint16_t  sectorsPerTrack;     uint16_t numHeads;
//         uint32_t  hiddenSectors;       uint32_t totalSectors32;
//     };
//     #pragma pack(pop)

//     const BPB* bpb = reinterpret_cast<const BPB*>(sector);
//     auto le16=[](uint16_t v){return v;}; auto le32=[](uint32_t v){return v;};

//     uint16_t bps = le16(bpb->bytesPerSector);
//     uint8_t spc  = bpb->sectorsPerCluster;
//     if (!bps || (bps & (bps-1)) || !spc || (spc & (spc-1))) return nullptr;

//     uint32_t totSec = le16(bpb->totalSectors16);
//     if (!totSec) totSec = le32(bpb->totalSectors32);

//     uint32_t fatSz = le16(bpb->fatSize16);
//     if (!fatSz) return nullptr;

//     uint32_t rootDirSec =
//         ((le16(bpb->rootEntryCount) * 32u) + (bps-1)) / bps;

//     uint32_t dataSec = totSec -
//                        (le16(bpb->reservedSectorCount) +
//                         bpb->numFATs * fatSz +
//                         rootDirSec);

//     uint32_t clusters = dataSec / spc;
//     if (clusters < 4085 || clusters >= 65525) return nullptr;  // not FAT16

//     auto* fs = new FAT16FileSystem();
//     fs->set_disk(disk);
//     return fs;
// }

// FileSystem* FAT32FileSystem::detect(BlockIODevice* disk)
// {
//     uint8_t sector[512];
//     if (!disk->read_sectors(0, 1, sector)) return nullptr;
//     if (sector[510] != 0x55 || sector[511] != 0xAA) return nullptr;

//     #pragma pack(push,1)
//     struct BPB {
//         uint8_t   jmpBoot[3];  char oem[8];
//         uint16_t  bytesPerSector;      uint8_t  sectorsPerCluster;
//         uint16_t  reservedSectorCount; uint8_t  numFATs;
//         uint16_t  rootEntryCount;      uint16_t totalSectors16;
//         uint8_t   media;               uint16_t fatSize16;
//         uint16_t  sectorsPerTrack;     uint16_t numHeads;
//         uint32_t  hiddenSectors;       uint32_t totalSectors32;
//         uint32_t  fatSize32;           // FAT32-specific
//         uint16_t  extFlags;            uint16_t fsVer;
//         uint32_t  rootCluster;         uint16_t fsInfo;
//         uint16_t  backupBoot;          uint8_t  reserved[12];
//     };
//     #pragma pack(pop)

//     const BPB* bpb = reinterpret_cast<const BPB*>(sector);
//     auto le16=[](uint16_t v){return v;}; auto le32=[](uint32_t v){return v;};

//     uint16_t bps = le16(bpb->bytesPerSector);
//     uint8_t  spc =  bpb->sectorsPerCluster;
//     if (!bps || (bps & (bps-1)) || !spc || (spc & (spc-1))) return nullptr;

//     uint32_t totSec = le16(bpb->totalSectors16);
//     if (!totSec) totSec = le32(bpb->totalSectors32);

//     uint32_t fatSz = le32(bpb->fatSize32);
//     if (!fatSz || bpb->fatSize16 != 0) return nullptr;   // must be FAT32 layout

//     uint32_t rootDirSec = 0;   // root directory is a normal cluster chain
//     uint32_t dataSec = totSec -
//                        (le16(bpb->reservedSectorCount) +
//                         bpb->numFATs * fatSz +
//                         rootDirSec);

//     uint32_t clusters = dataSec / spc;
//     if (clusters < 65525) return nullptr;                // not big enough

//     auto* fs = new FAT32FileSystem();
//     fs->set_disk(disk);
//     return fs;
// }

