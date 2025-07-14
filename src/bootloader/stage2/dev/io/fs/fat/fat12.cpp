#include "dev/io/fs/fat/fat.hpp"
#include "globals.hpp"
#include "stdio.hpp"
#include "string.hpp"
#include "memory.hpp"
#include "minmax.hpp"

FAT12FileSystem::FAT12FileSystem() {
    _sectorsPerFat = 0;
    _dataSectionLba = 0;
    _totalSectors = 0;
}

bool FAT12FileSystem::initialize() {
    if(!disk->read_sectors(0, 1, &_data.BS.BootSectorBytes)){
        stdio::printf(vga, "ERROR: FAT12: Failed to read boot sector\n");
        return false;
    }

    _data.FatCachePosition = 0xFFFFFFFF;

    _totalSectors = _data.BS.BootSector.total_sectors_16;
    if (_totalSectors == 0)
    { // fat32
        return false; // We are supposed to be in FAT12 not 32
    }

    if(_data.BS.BootSector.fat_size_16 == 0){
        return false; // again we are in fat 12 not supposed to be here;
    }
    _sectorsPerFat = _data.BS.BootSector.fat_size_16;

    uint32_t rootDirLba = _data.BS.BootSector.reserved_sector_count + _sectorsPerFat * _data.BS.BootSector.num_fats;
    uint32_t rootDirSize = sizeof(FAT_DirectoryEntry) * _data.BS.BootSector.root_entry_count;
    uint32_t rootDirSectors = (rootDirSize + _data.BS.BootSector.bytes_per_sector - 1) / _data.BS.BootSector.bytes_per_sector;
    _dataSectionLba = rootDirLba + rootDirSectors;

    _data.RootDirectory.Public.handle = ROOT_DIRECTORY_HANDLE;
    _data.RootDirectory.Public.is_directory = true;
    _data.RootDirectory.Public.position = 0;
    _data.RootDirectory.Public.size = sizeof(FAT_DirectoryEntry) * _data.BS.BootSector.root_entry_count;
    _data.RootDirectory.Opened = true;
    _data.RootDirectory.FirstCluster = rootDirLba;
    _data.RootDirectory.CurrentCluster = rootDirLba;
    _data.RootDirectory.CurrentSectorInCluster = 0;

    if(!disk->read_sectors(rootDirLba, 1, _data.RootDirectory.Buffer)){
        stdio::printf(vga, "ERROR: FAT12: Failed to read root directory\n");
        return false;
    }

    for (int i = 0; i < MAX_FILE_HANDLES; i++)
        _data.OpenedFiles[i].Opened = false;

    return true;

}

void FAT12FileSystem::shutdown() {

}

File* FAT12FileSystem::open(const char* path) {

    char name[MAX_PATH_SIZE];

    if(path[0] == '/')
        path++;

    File *current = &_data.RootDirectory.Public;

    while (*path)
    {
        // extract next file name from path
        bool isLast = false;
        const char *delim = strchr(path, '/');
        if (delim != NULL)
        {
            memcpy(name, path, delim - path);
            name[delim - path] = '\0';
            path = delim + 1;
        }
        else
        {
            unsigned len = strlen(path);
            memcpy(name, path, len);
            name[len] = '\0';
            path += len;
            isLast = true;
        }

        FAT_DirectoryEntry entry;

        if(FindFile(current, name, &entry)){
            this->close(current);

            // check if directory
            if (!isLast && (entry.Attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)
            {
                stdio::printf(vga, "FAT: not a directory\n");
                return NULL;
            }

            // open new directory entry
            current = OpenEntry(&entry);
        }
        else
        {
            this->close(current);

            stdio::printf(vga, "ERROR: FAT12: not found\n");
            return NULL;
        }

    }

    return current;

}

bool FAT12FileSystem::FindFile(File* current, char* name, FAT_DirectoryEntry* entryOut){

    char fatName[12];
    FAT_DirectoryEntry entry;

    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char *ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (ext != name + 11)
    {
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);
    }

    while (ReadEntry(current, &entry))
    {
        if (memcmp(fatName, entry.Name, 11) == 0)
        {
            *entryOut = entry;
            return true;
        }
    }

    return false;
}

bool FAT12FileSystem::ReadEntry(File* file, FAT_DirectoryEntry* dirEntry)
{
    uint32_t output = read(file, sizeof(FAT_DirectoryEntry), dirEntry) == sizeof(FAT_DirectoryEntry);
    if (output && dirEntry->Name[0] == 0)
        output = false;
    return output;
}

uint32_t FAT12FileSystem::NextCluster(uint32_t currentCluster)
{
    uint32_t fatIndex = currentCluster * 3 / 2;

    // Make sure cache has the right number
    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if (fatIndexSector < _data.FatCachePosition || fatIndexSector >= _data.FatCachePosition + FAT_CACHE_SIZE)
    {
        ReadFat(fatIndexSector);
        _data.FatCachePosition = fatIndexSector;
    }

    fatIndex -= (_data.FatCachePosition * SECTOR_SIZE);

    uint32_t nextCluster;
    if (currentCluster % 2 == 0)
        nextCluster = (*(uint16_t *)(_data.FatCache + fatIndex)) & 0x0FFF;
    else
        nextCluster = (*(uint16_t *)(_data.FatCache + fatIndex)) >> 4;

    if (nextCluster >= 0xFF8)
    {
        nextCluster |= 0xFFFFF000;
    }

    return nextCluster;
}

bool FAT12FileSystem::ReadFat(uint32_t lbaIndex)
{
    return disk->read_sectors(_data.BS.BootSector.reserved_sector_count + lbaIndex, FAT_CACHE_SIZE, _data.FatCache);
}

File* FAT12FileSystem::OpenEntry(FAT_DirectoryEntry *entry)
{

    // find empty handle
    int handle = -1;
    for (int i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
    {
        if (!_data.OpenedFiles[i].Opened)
            handle = i;
    }

    // out of handles
    if (handle < 0)
    {
        stdio::printf(vga, "ERROR: FAT12: out of file handles\n");
        return 0;
    }

    // setup vars
    FAT_FileData *fd = &_data.OpenedFiles[handle];
    fd->Public.handle = handle;
    fd->Public.is_directory = (entry->Attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->Public.position = 0;
    fd->Public.size = entry->Size;
    fd->FirstCluster = entry->FirstClusterLow + ((uint32_t)entry->FirstClusterHigh << 16);
    fd->CurrentCluster = fd->FirstCluster;
    fd->CurrentSectorInCluster = 0;

    if (fd->CurrentCluster < 2 || fd->CurrentCluster >= 0xFF8) {
        stdio::printf(vga, "ERROR: FAT12: invalid cluster %u\n", fd->CurrentCluster);
        return 0;
    }

    if (!disk->read_sectors(this->clusterToLba(fd->CurrentCluster), 1, fd->Buffer))
    {
        stdio::printf(vga, "FAT: open entry failed - read error cluster=%u lba=%u\n", fd->CurrentCluster, this->clusterToLba(fd->CurrentCluster));
        for (int i = 0; i < 11; i++)
            stdio::printf(vga, "%c", entry->Name[i]);
        stdio::printf(vga, "\n");
        return 0;
    }

    fd->Opened = true;
    return &fd->Public;
}

uint32_t FAT12FileSystem::clusterToLba(uint32_t cluster)
{
    return _dataSectionLba + (cluster - 2) * _data.BS.BootSector.sectors_per_cluster;
}

uint32_t FAT12FileSystem::read(File* file, uint32_t byteCount, void* dataOut) {
    // get file data
    FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                           ? &_data.RootDirectory
                           : &_data.OpenedFiles[file->handle];

    if(!fd->Opened){
        return 0;
    }

    uint8_t *u8DataOut = (uint8_t *)dataOut;

    // don't read past the end of the file
    if (!fd->Public.is_directory || (fd->Public.is_directory && fd->Public.size != 0))
        byteCount = min(byteCount, fd->Public.size - fd->Public.position);

    while (byteCount > 0)
    {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->Public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOut, fd->Buffer + fd->Public.position % SECTOR_SIZE, take);
        u8DataOut += take;
        fd->Public.position += take;
        byteCount -= take;

        // printf("leftInBuffer=%lu take=%lu\r\n", leftInBuffer, take);
        // See if we need to read more data
        if (leftInBuffer == take)
        {
            // calculate next cluster & sector to read
            if (++fd->CurrentSectorInCluster >= _data.BS.BootSector.sectors_per_cluster)
            {
                fd->CurrentSectorInCluster = 0;
                fd->CurrentCluster = NextCluster(fd->CurrentCluster);
            }

            if (fd->CurrentCluster >= 0xFFFFFFF8)
            {
                // Mark end of file
                fd->Public.size = fd->Public.position;
                break;
            }

            // read next sector
            if (!disk->read_sectors(clusterToLba(fd->CurrentCluster) + fd->CurrentSectorInCluster, 1, fd->Buffer))
            {
                stdio::printf(vga, "ERROR: FAT12: read error!\n");
                break;
            }
        }
    }

    return u8DataOut - (uint8_t *)dataOut;
}

void FAT12FileSystem::close(File* file) {
    if (file->handle == ROOT_DIRECTORY_HANDLE)
    {
        file->position = 0;
        _data.RootDirectory.CurrentCluster = _data.RootDirectory.FirstCluster;
    }
    else
    {
        _data.OpenedFiles[file->handle].Opened = false;
    }
}

bool FAT12FileSystem::seek(File* file, uint32_t position) {
    // Validate bounds
    if (!file || (!file->is_directory && position > file->size))
        return false;

    // Get file data
    FAT_FileData* fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                           ? &_data.RootDirectory
                           : &_data.OpenedFiles[file->handle];

    if (!fd->Opened)
        return false;

    // Set new position
    fd->Public.position = position;

    // Calculate new cluster/sector
    uint32_t cluster = fd->FirstCluster;
    uint32_t offset = position;
    uint32_t clusterSize = _data.BS.BootSector.sectors_per_cluster * SECTOR_SIZE;

    while (offset >= clusterSize) {
        cluster = NextCluster(cluster);
        if (cluster >= 0xFFFFFFF8)
            return false;
        offset -= clusterSize;
    }

    fd->CurrentCluster = cluster;
    fd->CurrentSectorInCluster = offset / SECTOR_SIZE;

    // Read sector into buffer
    return disk->read_sectors(clusterToLba(cluster) + fd->CurrentSectorInCluster, 1, fd->Buffer);
}
