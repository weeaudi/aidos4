
#pragma once

#include "dev/io/io.hpp"
#include "dev/driver.hpp"

/**
 * @file fs.hpp
 * @brief Abstractions for simple file systems used by the bootloader.
 */

/**
 * @brief Representation of an opened file or directory.
 */
struct File {
    uint8_t handle;
    bool is_directory;
    uint32_t size;
    uint32_t position;
};

/**
 * @brief Abstract base class for a file system implementation.
 */
class FileSystem : public Driver {
public:
    /** Associate the underlying block device. */
    virtual void set_disk(BlockIODevice* device) {
        disk = device;
    }

    /** Initialise the file system structures. */
    virtual bool initialize() = 0;

    /** Open a file at the given path. */
    virtual File* open(const char* path) = 0;

    /** Read bytes from an open file. */
    virtual uint32_t read(File* file, uint32_t byteCount, void* out) = 0;

    /** Move the file pointer. */
    virtual bool seek(File* file, uint32_t position) = 0;

    /** Close an open file. */
    virtual void close(File* file) = 0;

    /** Virtual destructor. */
    virtual ~FileSystem() {}

protected:
    BlockIODevice* disk = nullptr; ///< Underlying disk device
};

using FileSystemDetector = FileSystem* (*)(BlockIODevice*);
FileSystem* detectAndMount(BlockIODevice* disk);