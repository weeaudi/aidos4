#pragma once

#include "dev/io/io.hpp"
#include "dev/driver.hpp"

struct File {
    uint8_t handle;          
    bool is_directory;
    uint32_t size;
    uint32_t position;
};

class FileSystem : public Driver {
public:
    virtual void set_disk(BlockIODevice* device) {
        disk = device;
    }

    virtual bool initialize() = 0;

    virtual File* open(const char* path) = 0;
    virtual uint32_t read(File* file, uint32_t byteCount, void* out) = 0;
    virtual bool seek(File* file, uint32_t position) = 0;
    virtual void close(File* file) = 0;

    virtual ~FileSystem() {}

protected:
    BlockIODevice* disk = nullptr;
};

using FileSystemDetector = FileSystem* (*)(BlockIODevice*);
FileSystem* detectAndMount(BlockIODevice* disk);