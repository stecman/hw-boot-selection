#pragma once

#include <stdint.h>

// Convert a value to a 16-bit little-endian array listing
#define WBVAL(x) ((x) & 0xFF), (((x) >> 8) & 0xFF)

// Convert a value to a 32-bit little-endian array listing
#define QBVAL(x) ((x) & 0xFF), (((x) >> 8) & 0xFF), (((x) >> 16) & 0xFF), (((x) >> 24) & 0xFF)

#define FAT_MEDIA_FIXED_DISK 0xF8 // Media descriptor for Fixed Disk
#define FAT_NAME_LEN 8
#define FAT_EXT_LEN 3

enum FatFileAttributes {
    kFatAttr_ReadOnly = 0x1,
    kFatAttr_Hidden = 0x2,
    kFatAttr_System = 0x4,
    kFatAttr_VolumeLabel = 0x8,
    kFatAttr_Subdirectory = 0x10,
    kFatAttr_Archive = 0x20,
    kFatAttr_LongFileName = 0x0F,
};

enum FatOrdinalFlags {
    kFat_LastLFN = 0x40,
    kFat_DeletedLFN = 0x80,
};

struct FatDirEntry {
    char name[FAT_NAME_LEN];
    char ext[FAT_EXT_LEN];
    uint8_t attrs;    // Flags
    uint8_t type;     // Type (reserved)
    uint8_t ctime_ms; // Creation time, milliseconds
    uint16_t ctime;   // Creation time
    uint16_t cdate;   // Creation date
    uint16_t adate;   // Last access date
    uint16_t ea_index; // EA-Index
    uint16_t mtime;   // modified time
    uint16_t mdate;   // modified date
    uint16_t start;   // first cluster
    uint32_t size;    // file size (in bytes)

} __attribute__ ((packed));

/**
 * Write a long filename directory entry to an output pointer
 *
 * The long filename will use 32 bytes for every 11 characters in `name`
 *
 * Returns the number of bytes written to the output pointer
 */
uint8_t fat_write_lfn(const char* name, const struct FatDirEntry *direntry,  uint8_t* output);

/**
 * Write a directory entry to an output pointer
 *
 * Returns the number of bytes written to the output pointer (always 32)
 */
uint8_t fat_write_dir(const struct FatDirEntry *direntry, uint8_t* output);