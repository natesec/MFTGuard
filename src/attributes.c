#include <stdio.h>

#include "attributes.h"
#include "utils.h"

#define ERROR_MARKER "[!] "

/** First 2 attribute header fields*/
#define ATTRIBUTE_TYPE_OFFSET 0x00         /** 4 bytes */
#define ATTRIBUTE_LENGTH_OFFSET 0x04       /** 4 bytes */

#define ATTRIBUTE_DEFAULT_HEADER_SIZE 0x10 /** 16 bytes */

bool attributes_walk(
    const uint8_t *record,
    uint32_t record_size,
    uint16_t attribute_offset
)
{
    if (record == NULL)
    {
        return false;
    }

    if (attribute_offset >= record_size)
    {
        fprintf(stderr, ERROR_MARKER "attribute walk failed: invalid sizes\n");
        return false;
    }

    uint32_t offset = attribute_offset;

    while (offset < record_size)
    {
        if (record_size - offset < ATTRIBUTE_DEFAULT_HEADER_SIZE)
        {
            fprintf(stderr, ERROR_MARKER "attribute walk failed: incomplete attr header\n");
            return false;
        }

        uint32_t attribute_type = read_u32_le(record + offset + ATTRIBUTE_TYPE_OFFSET);

        if (attribute_type == ATTRIBUTE_TYPE_END)
        {
            return true;
        }

        uint32_t attribute_length = read_u32_le(record + offset + ATTRIBUTE_LENGTH_OFFSET);

        if (attribute_length == 0)
        {
            fprintf(stderr, ERROR_MARKER "attribute walk failed: attr length = 0\n");
            return false;
        }

        if (attribute_length > record_size - offset)
        {
            fprintf(stderr, ERROR_MARKER "attribute walk failed: attr exceeds record bounds\n");
            return false;
        }

        offset += attribute_length;
    }

    fprintf(stderr, ERROR_MARKER "attribute walk failed: end marker not found\n");
    return false;
}
