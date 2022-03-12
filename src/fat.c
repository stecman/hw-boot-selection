#include "fat.h"

#include <string.h>

/**
 * Calculate checksum from a 8.3 short filename for use in LFN entries
 * Copied from fs/fat/fat.h in the Linux source tree
 */
static inline uint8_t _fat_checksum(const char *name)
{
    uint8_t s = name[0];

    s = (s<<7) + (s>>1) + name[1];  s = (s<<7) + (s>>1) + name[2];
    s = (s<<7) + (s>>1) + name[3];  s = (s<<7) + (s>>1) + name[4];
    s = (s<<7) + (s>>1) + name[5];  s = (s<<7) + (s>>1) + name[6];
    s = (s<<7) + (s>>1) + name[7];  s = (s<<7) + (s>>1) + name[8];
    s = (s<<7) + (s>>1) + name[9];  s = (s<<7) + (s>>1) + name[10];

    return s;
}

static uint8_t _write_utf16_chars(const char* name, uint8_t* index, uint8_t length,
                                  uint8_t writeCount, uint8_t* output)
{
    // Write little-endian UTF-16 characters
    for (uint8_t i = writeCount; i != 0; --i) {
        char value = 0xFF;
        if (*index <= length) {
            value = name[*index];

            (*output++) = value;
            (*output++) = 0x0;

            ++(*index);
        } else {
            (*output++) = 0xFF;
            (*output++) = 0xFF;
        }
    }

    return writeCount * 2;
}

uint8_t fat_write_lfn(const char* name, const struct FatDirEntry *direntry, uint8_t* output)
{
    const uint8_t kCharsPerEntry = 13;

    // Calculate number of entries required, rounding up
    const size_t length = strlen(name);
    const uint8_t numEntries = (length + (kCharsPerEntry - 1)) / kCharsPerEntry;

    // Calculate checksum from the short name in the target directory entry
    const uint8_t checksum = _fat_checksum((const char*) direntry);

    // Write LFN entries in last-to-first order
    for (uint8_t ordinal = numEntries; ordinal != 0; --ordinal)
    {
        // Index to the part of the name we're writing in this entry
        uint8_t index = ((ordinal - 1) * kCharsPerEntry);

        // Sequence number, combined with flag if this is that last entry
        *output = ordinal | (kFat_LastLFN * (ordinal == numEntries) );
        ++output;


        // First character chunk
        output += _write_utf16_chars(name, &index, length, 5, output);

        // Attributes, type and checksum bytes
        output[0] = kFatAttr_LongFileName;
        output[1] = 0x0;
        output[2] = checksum;
        output += 3;

        // Second character chunk
        output += _write_utf16_chars(name, &index, length, 6, output);

        // First cluster - (always 0x0000)
        output[0] = 0x0;
        output[1] = 0x0;
        output += 2;

        // Third character chunk
        output += _write_utf16_chars(name, &index, length, 2, output);
    }

    return sizeof(struct FatDirEntry) * numEntries;
}

uint8_t fat_write_dir(const struct FatDirEntry *direntry, uint8_t* output)
{
    const uint8_t kShortNameLength = FAT_NAME_LEN + FAT_EXT_LEN;

    uint8_t* data = (uint8_t*) direntry;
    memcpy(output, data, sizeof(struct FatDirEntry));

    return sizeof(struct FatDirEntry);
}

/**
 * For a good explanation of the DOS date/time format used here, see:
 * https://stackoverflow.com/a/15763512
 */
uint16_t fat_date(const uint16_t year, const uint8_t month, const uint8_t day)
{
    return ((year - 1980) << 9) | // Year is stored as an offset from 1980
           (month << 5) |
           (day);
}