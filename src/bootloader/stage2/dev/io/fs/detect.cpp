#include "dev/io/fs/fs.hpp"
#include "dev/io/fs/fat/fat.hpp"

static FileSystemDetector detectors[] = {
    &FAT12FileSystem::detect,
    //&FAT16FileSystem::detect,
    //&FAT32FileSystem::detect,
    nullptr
};

FileSystem* detectAndMount(BlockIODevice* disk) {
    for (int i = 0; detectors[i]; ++i) {
        FileSystem* fs = detectors[i](disk);
        if (fs && fs->initialize()) {
            return fs;  // found and initialized
        }
        if (fs) delete fs;  // detected, but init failed
    }
    return nullptr;
}