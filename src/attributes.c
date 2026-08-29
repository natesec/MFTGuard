#include <stdio.h>

#include "attributes.h"
#include "utils.h"
#include "timestamps.h"

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

/**
 * @brief Parses the attribute of type $STANDARD_INFORMATION.
 * @param value Pointer to the raw $STANDARD_INFORMATION attribute value.
 * @param value_length Size of the attribute value in bytes.
 * @param si Pointer to the $SI struct to receive the parsed values.
 * @return True if parsed successfully, false otherwise.
 */
static bool attribute_parse_standard_information(const uint8_t *value, uint32_t value_length, standard_information *si);

/**
 * @brief Parses the $FILE_NAME attribute type.
 * @param value Pointer to the raw $FILE_NAME attribute value.
 * @param value_length Size of the attribute value in bytes.
 * @param fn Pointer to the $FN struct to receive parsed values.
 * @return true if parsed successfully, false otherwise.
 */
static bool attribute_parse_file_name(const uint8_t *value, uint32_t value_length, file_name_information *fn);

bool attributes_walk(
    const uint8_t *record,
    uint32_t record_size,
    uint16_t attribute_offset,
    record_metadata *metadata
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
        attribute_header header;
        resident_attribute_header resident_header;
        const uint8_t *value;

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

            if (metadata->has_standard_information)
            {
                fprintf(stderr, ERROR_MARKER "record contains multiple SI structures\n");
                break;
            }

            printf(MESSAGE_MARKER "----$STANDARD_INFORMATION found\n");

            if (header.non_resident != 0)
            {
                fprintf(stderr, ERROR_MARKER "$SI is non-resident\n");
                return false;
            }

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

            if (resident_header.value_length < STANDARD_INFORMATION_TIMESTAMP_SIZE)
            {
                fprintf(stderr, ERROR_MARKER "$SI does not have four timestamps\n");
                return false;
            }

            value = record + offset + resident_header.value_offset;

            /** standard_information si; **/

            if (!attribute_parse_standard_information(value, resident_header.value_length, &metadata->si))
            {
                return false;
            }

            metadata->has_standard_information = true;

            break;

        case ATTRIBUTE_TYPE_FILE_NAME:

            if (metadata->has_file_name_information)
            {
                fprintf(stderr, ERROR_MARKER "record contains multiple FN structures\n");
                break;
            }

            printf(MESSAGE_MARKER "----$FILE_NAME found\n");

            if (header.non_resident != 0)
            {
                fprintf(stderr, ERROR_MARKER "$FN is non-resident\n");
                return false;
            }

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

            value = record + offset + resident_header.value_offset;

            /** file_name_information fn; **/

            if (!attribute_parse_file_name(value, resident_header.value_length, &metadata->fn))
            {
                return false;
            }

            metadata->has_file_name_information = true;

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

static bool attribute_parse_standard_information(const uint8_t *value,
    uint32_t value_length, standard_information *si)
{
    if (value == NULL || si == NULL)
    {
        fprintf(stderr, ERROR_MARKER "$SI parsing failed\n");
        return false;
    }

    si->creation_time = read_u64_le(value + 0x00);
    si->modified_time = read_u64_le(value + 0x08);
    si->mft_modified_time = read_u64_le(value + 0x10);
    si->accessed_time = read_u64_le(value + 0x18);

    return true;
}

static bool attribute_parse_file_name(const uint8_t *value, uint32_t value_length, file_name_information *fn)
{
    if (value == NULL || fn == NULL)
    {
        fprintf(stderr, ERROR_MARKER "$FN parsing failed\n");
        return false;
    }

    fn->creation_time = read_u64_le(value + 0x08);
    fn->modified_time = read_u64_le(value + 0x10);
    fn->mft_modified_time = read_u64_le(value + 0x18);
    fn->accessed_time = read_u64_le(value + 0x20);

    fn->filename_length = value[0x40];
    fn->filename_namespace = value[0x41];

    uint32_t filename_size = (uint32_t)fn->filename_length * 2;

    if (0x42 + filename_size > value_length)
    {
        fprintf(stderr, ERROR_MARKER "$FN filename size exceeds value size\n");
        return false;
    }

    fn->filename = value + 0x42;

    return true;
}
