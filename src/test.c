#include <stdio.h>
#include "fat.h"

// Filesystem size is (SECTOR_COUNT * SECTOR_SIZE) in bytes
// Note that sector size cannot be changed as usb_msc_init defines it
#define SECTOR_COUNT 512
#define SECTOR_SIZE 512
#define BYTES_PER_SECTOR 512
#define SECTORS_PER_CLUSTER 4
#define RESERVED_SECTORS 1
#define FAT_COPIES 2
#define ROOT_ENTRIES 512
#define ROOT_ENTRY_LENGTH 32
#define FILEDATA_START_CLUSTER 3
#define DATA_REGION_SECTOR (RESERVED_SECTORS + FAT_COPIES + (ROOT_ENTRIES * ROOT_ENTRY_LENGTH) / BYTES_PER_SECTOR)
#define FILEDATA_START_SECTOR (DATA_REGION_SECTOR + (FILEDATA_START_CLUSTER - 2) * SECTORS_PER_CLUSTER)
#define FILEDATA_SECTOR_COUNT 1

static struct FatDirEntry testEntry = {
    .name = "HELLO_~1",
    .ext = "TXT",
    .start = 0x69,
    .size = 32,
};

static void generate_dir_sector(uint8_t* output)
{
    output += fat_write_lfn("hello_world.txt", &testEntry, output);
    output += fat_write_dir(&testEntry, output);
}

int main()
{
    uint8_t buffer[2048] = { 0 };

    generate_dir_sector((uint8_t*) buffer + 0x600);

    fwrite(buffer, sizeof(buffer), 1, stdout);
    // fwrite(&testEntry, sizeof(testEntry), 1, stdout);

    return 0;
}