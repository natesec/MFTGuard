#include <stdio.h>

#include "fixup.h"
#include "utils.h"

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

    return true;
}