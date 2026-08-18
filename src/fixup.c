#include <stdio.h>

#include "fixup.h"

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

    return true;
}