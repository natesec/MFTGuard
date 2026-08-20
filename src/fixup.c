#include <stdio.h>

#include "fixup.h"
#include "utils.h"

#define ERROR_MARKER "[!] "

bool fixup_apply(
    uint8_t *record, 
    uint32_t record_size, 
    uint32_t sector_size, 
    uint16_t usa_offset, 
    uint16_t usa_count
)
{
    if (record == NULL)
    {
        fprintf(stderr, ERROR_MARKER "fixup failed: record NULL\n");
        return false;
    }

    if (record_size == 0)
    {
        fprintf(stderr, ERROR_MARKER "fixup failed: record size = 0\n");
        return false;
    }

    if (sector_size == 0)
    {
        fprintf(stderr, ERROR_MARKER "fixup failed: sector size = 0\n");
        return false;
    }

    if (record_size % sector_size !=0)
    {
        fprintf(
            stderr,
            ERROR_MARKER "fixup failed: record size not evenly divisible by sector size\n"
        );
        return false;
    }

    uint32_t sector_count = record_size / sector_size;
    uint32_t expected_usa_count = sector_count + 1;

    if (usa_count != expected_usa_count)
    {
        fprintf(stderr, ERROR_MARKER "fixup failed: unexpected sector count");
        return false;
    }

    uint32_t usa_size = (uint32_t)usa_count * sizeof(uint16_t);

    if (usa_size > record_size - usa_offset)
    {
        fprintf(stderr, ERROR_MARKER "fixup failed: USA size too large\n");
        return false;
    }

    // USA[0] = USN
    const uint8_t *usa = record + usa_offset;
    uint16_t usn = read_u16_le(usa);

    // Process each sector end
    for (uint32_t i = 0; i< sector_count; i++)
    {
        uint32_t sector_end = ((i+1) * sector_size) - sizeof(uint16_t);

        uint16_t sector_usn = read_u16_le(record + sector_end);

        // Final WORD of sector must = USN
        if (sector_usn != usn)
        {
            fprintf(stderr, ERROR_MARKER "fixup failed: USN mismatch");
            return false;
        }

        // USA[i + 1] = original WORD
        uint16_t replacement = read_u16_le(
            usa + ((i+1) * sizeof(uint16_t))
        );

        // Restore original bytes in order of little-endian
        record[sector_end] = (uint8_t)(replacement & 0xFF);
        record[sector_end + 1] = (uint8_t)(replacement >> 8);
    }

    return true;
}