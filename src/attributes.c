#include <stdio.h>

#include "attributes.h"
#include "utils.h"

#define ERROR_MARKER "[!] "
#define MESSAGE_MARKER "[*] "

/** First 2 attribute header fields*/
#define ATTRIBUTE_TYPE_OFFSET 0x00         /** 4 bytes */
#define ATTRIBUTE_LENGTH_OFFSET 0x04       /** 4 bytes */

#define ATTRIBUTE_DEFAULT_HEADER_SIZE 0x10 /** 16 bytes */

/**
 * @brief Parses the attributes of a header and populates the attribute_header
 *        struct, used to store common attribute header fields.
 * @param buffer Pointer, contains attribute header in a given record.
 * @param header Pointer to an attribute header structure to store common fields.
 * @return true if the attribute header was parsed, false otherwise.
 */
static bool attribute_parse_header(const uint8_t *buffer, attribute_header *header);

/**
 * @brief Parses the resident header of an attribute and populates the resident
 *        attribute header fields.
 * @param buffer Pointer to the buffer containing the resident header.
 * @param header Pointer to the resident attribute header struct to populate.
 */
static bool attribute_parse_resident_header(const uint8_t *buffer, resident_attribute_header *header);

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

    attribute_header header;

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

        if (!attribute_parse_header(record + offset, &header))
        {
            return false;
        }

        switch (header.type)
        {
        case ATTRIBUTE_TYPE_STANDARD_INFORMATION:
            printf(MESSAGE_MARKER "----$STANDARD_INFORMATION found\n");

            if (header.non_resident != 0)
            {
                fprintf(stderr, ERROR_MARKER "$SI is non-resident\n");
                return false;
            }

            resident_attribute_header resident_header;

            if (!attribute_parse_resident_header(
                record + offset + ATTRIBUTE_DEFAULT_HEADER_SIZE,
                &resident_header))
            {
                return false;
            }

            if (resident_header.value_offset > header.length)
            {
                fprintf(stderr, ERROR_MARKER "$SI value offset exceeds attribute length\n");
                return false;
            }

            if (resident_header.value_length > header.length - resident_header.value_offset)
            {
                fprintf(stderr, ERROR_MARKER "$SI value exceeds attribute bounds\n");
                return false;
            }

            if (resident_header.value_length < 0x20)
            {
                fprintf(stderr, ERROR_MARKER "$SI does not have four timestamps\n");
                return false;
            }

            const uint8_t *value = record + offset + resident_header.value_offset;

            break;

        case ATTRIBUTE_TYPE_FILE_NAME:
            printf(MESSAGE_MARKER "----$FILE_NAME found\n");

            if (header.non_resident != 0)
            {
                fprintf(stderr, ERROR_MARKER "$FN is non-resident\n");
                return false;
            }

            resident_attribute_header resident_header;

            if (!attribute_parse_resident_header(
                record + offset + ATTRIBUTE_DEFAULT_HEADER_SIZE,
                &resident_header))
            {
                return false;
            }

            if (resident_header.value_offset > header.length)
            {
                fprintf(stderr, ERROR_MARKER "$FN value offset exceeds attribute length\n");
                return false;
            }

            if (resident_header.value_length > header.length - resident_header.value_offset)
            {
                fprintf(stderr, ERROR_MARKER "$FN value exceeds attribute bounds\n");
                return false;
            }

            break;

        case ATTRIBUTE_TYPE_DATA:
            printf(MESSAGE_MARKER "----$DATA found\n");
            break;

        case ATTRIBUTE_TYPE_BITMAP:
            printf(MESSAGE_MARKER "----$BITMAP found\n");
            break;

        default:
            printf(MESSAGE_MARKER "----Unknown type %u\n", header.type);
            break;
        }

        offset += attribute_length;
    }

    fprintf(stderr, ERROR_MARKER "attribute walk failed: end marker not found\n");
    return false;
}

static bool attribute_parse_header(
    const uint8_t *buffer, attribute_header *header)
{
    if (buffer == NULL || header == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to parse attribute header\n");
        return false;
    }

    header->type = read_u32_le(buffer + 0x00);
    header->length = read_u32_le(buffer + 0x04);
    header->non_resident = buffer[0x08];
    header->name_length = buffer[0x09];
    header->name_offset = read_u16_le(buffer + 0x0A);
    header->flags = read_u16_le(buffer + 0x0C);
    header->attribute_id = read_u16_le(buffer + 0x0E);

    return true;
}

static bool attribute_parse_resident_header(const uint8_t *buffer, resident_attribute_header *header)
{
    if (buffer == NULL || header == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to parse resident attribute header\n");
        return false;
    }

    header->value_length = read_u32_le(buffer + 0x00);
    header->value_offset = read_u16_le(buffer + 0x04);
    header->indexed = buffer[0x06];
    header->padding = buffer[0x07];

    return true;
}
