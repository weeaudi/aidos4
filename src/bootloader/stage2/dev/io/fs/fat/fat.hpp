#include "dev/io/fs/fs.hpp"

/**
 * @file fat.hpp
 * @brief Minimal FAT12 file system implementation used by the bootloader.
 */

#define SECTOR_SIZE 512       ///< Size of a disk sector in bytes
#define MAX_PATH_SIZE 256     ///< Maximum path length supported
#define MAX_FILE_HANDLES 10   ///< Maximum number of simultaneously opened files
#define ROOT_DIRECTORY_HANDLE (MAX_FILE_HANDLES + 1)
#define FAT_CACHE_SIZE 5      ///< Number of sectors kept in the FAT cache

#pragma pack(push, 1)

/**
 * @brief On-disk layout of the FAT boot sector.
 */
struct FAT_BootSector {
    // --- Jump and OEM Name ---
    uint8_t     jump_instruction[3];      // 0x00
    char        oem_name[8];              // 0x03

    // --- BIOS Parameter Block (common for all FATs) ---
    uint16_t    bytes_per_sector;         // 0x0B
    uint8_t     sectors_per_cluster;      // 0x0D
    uint16_t    reserved_sector_count;    // 0x0E
    uint8_t     num_fats;                 // 0x10
    uint16_t    root_entry_count;         // 0x11 (FAT12/16 only)
    uint16_t    total_sectors_16;         // 0x13
    uint8_t     media_descriptor;         // 0x15
    uint16_t    fat_size_16;              // 0x16 (FAT12/16 only)
    uint16_t    sectors_per_track;        // 0x18
    uint16_t    num_heads;                // 0x1A
    uint32_t    hidden_sectors;           // 0x1C
    uint32_t    total_sectors_32;         // 0x20

    // --- FAT Type-specific EBR ---
    union {
        // FAT12/16 EBR
        struct {
            uint8_t     drive_number;         // 0x24
            uint8_t     reserved1;            // 0x25
            uint8_t     boot_signature;       // 0x26 (0x29 indicates next fields are present)
            uint32_t    volume_id;            // 0x27
            char        volume_label[11];     // 0x2B
            char        fs_type[8];           // 0x36
            uint8_t     boot_code[448];       // 0x3E
        } fat16;

        // FAT32 EBR
        struct {
            uint32_t    fat_size_32;          // 0x24
            uint16_t    ext_flags;            // 0x28
            uint16_t    fs_version;           // 0x2A
            uint32_t    root_cluster;         // 0x2C
            uint16_t    fs_info;              // 0x30
            uint16_t    backup_boot_sector;   // 0x32
            uint8_t     reserved[12];         // 0x34
            uint8_t     drive_number;         // 0x40
            uint8_t     reserved1;            // 0x41
            uint8_t     boot_signature;       // 0x42
            uint32_t    volume_id;            // 0x43
            char        volume_label[11];     // 0x47
            char        fs_type[8];           // 0x52
            uint8_t     boot_code[420];       // 0x5A
        } fat32;
    };

    // --- Boot Sector Signature ---
    uint16_t    signature;                 // 0x1FE (should be 0xAA55)
};

/**
 * @brief Directory entry as stored in the FAT file system.
 */
struct FAT_DirectoryEntry
{
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
};

/**
 * @brief Attribute flags used by FAT directory entries.
 */
enum FAT_Attributes
{
    FAT_ATTRIBUTE_READ_ONLY         = 0x01,
    FAT_ATTRIBUTE_HIDDEN            = 0x02,
    FAT_ATTRIBUTE_SYSTEM            = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID         = 0x08,
    FAT_ATTRIBUTE_DIRECTORY         = 0x10,
    FAT_ATTRIBUTE_ARCHIVE           = 0x20,
    FAT_ATTRIBUTE_LFN               = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

#pragma pack(pop)

/**
 * @brief Internal state associated with an open file.
 */
typedef struct
{
    uint8_t Buffer[SECTOR_SIZE];
    File Public;
    bool Opened;
    uint32_t FirstCluster;
    uint32_t CurrentCluster;
    uint32_t CurrentSectorInCluster;

} FAT_FileData;

/**
 * @brief Global state for a mounted FAT12 file system.
 */
typedef struct
{
    union
    {
        FAT_BootSector BootSector;
        uint8_t BootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData RootDirectory;

    FAT_FileData OpenedFiles[MAX_FILE_HANDLES + 1];

    uint8_t FatCache[FAT_CACHE_SIZE * SECTOR_SIZE];
    uint32_t FatCachePosition;

} FAT_Data;

/**
 * @brief FAT12 file system driver used by the bootloader.
 */
class FAT12FileSystem : public FileSystem {
public:

    /** Construct an empty file system object. */
    FAT12FileSystem();

    /** Attempt to detect a FAT12 file system on the given disk. */
    static FileSystem* detect(BlockIODevice* disk);

    /** Initialise internal structures. */
    bool initialize();
    /** Release any resources held by the file system. */
    void shutdown();

    /** Open a file at the given path. */
    File* open(const char* path);
    /** Read data from a file. */
    uint32_t read(File* file, uint32_t byteCount, void* out);
    /** Move the read pointer of a file. */
    bool seek(File* file, uint32_t pointer);
    /** Close an opened file. */
    void close(File* file);

private:
    FAT_Data _data;
    uint32_t _dataSectionLba;
    uint32_t _totalSectors;
    uint32_t _sectorsPerFat;

    uint32_t clusterToLba(uint32_t cluster);
    bool FindFile(File* Current, char* name, FAT_DirectoryEntry* out);
    File* OpenEntry(FAT_DirectoryEntry* entry);
    bool ReadEntry(File* file, FAT_DirectoryEntry* entryOut);
    uint32_t NextCluster(uint32_t current_cluster);
    bool ReadFat(uint32_t lbaIndex);

};

// class FAT16FileSystem : public FileSystem {
// public:
//     static FileSystem* detect(BlockIODevice* disk);

//     bool initialize();
//     void shutdown();

//     File* open(const char* path);
//     virtual uint32_t read(File* file, uint32_t byteCount, void* out);
//     virtual bool close(File* file);
// };

// class FAT32FileSystem : public FileSystem {
// public:
//     static FileSystem* detect(BlockIODevice* disk);

//     bool initialize();
//     void shutdown();

//     File* open(const char* path);
//     virtual uint32_t read(File* file, uint32_t byteCount, void* out);
//     virtual bool close(File* file);

// };