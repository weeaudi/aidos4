#include "dev/io/text/vga/vga.hpp"
#include "dev/io/disk/ata.hpp"
#include "stdio.hpp"
#include "globals.hpp"
#include "dev/io/fs/fs.hpp"

VGADriver vga;
ATADrive ata;

#define HEADS 2
#define SECTORS 18

extern "C" void stage2_main() {
    vga.initialize();

    if(!ata.initialize()){
        stdio::printf(vga, "ERROR: FAILED TO INITIALIZE ATA DRIVER");
        while(true){}
    }

    FileSystem* fs = detectAndMount(static_cast<BlockIODevice*>(&ata));

    if(fs == nullptr){
        stdio::printf(vga, "ERROR: FAILED TO INITIALIZE FILESYSTEM ON DRIVE");
        while(true){}
    }

    File* hello = fs->open("/hello.txt");

    uint8_t buffer[512];

    if(fs->read(hello, 512, &buffer) == 0){
        stdio::printf(vga, "ERROR: could not read from hello.txt\n");
        while(true){}
    }

    stdio::printf(vga, "Stage 2 cpp loaded!");

    while(true){}
}
