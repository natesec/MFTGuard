#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include <stdbool.h>
#include <stdint.h>

/** NTFS attribute type that marks the end of the attribute list. */
#define ATTRIBUTE_TYPE_END 0xFFFFFFFF

#define ATTRIBUTE_TYPE_STANDARD_INFORMATION 0x10
#define ATTRIBUTE_TYPE_FILE_NAME 0x30
#define ATTRIBUTE_TYPE_DATA 0x80
#define ATTRIBUTE_TYPE_BITMAP 0xB0

/**
 * @brief Represents an MFT record attribute header.
 */
typedef struct
{
    uint32_t type;         /** 0x00 */
    uint32_t length;       /** 0x04 */
    uint8_t non_resident;  /** 0x08 */
    uint8_t name_length;   /** 0x09 */
    uint16_t name_offset;  /** 0x0A */
    uint16_t flags;        /** 0x0C */
    uint16_t attribute_id; /** 0x0E */
} attribute_header;

/**
 * @brief Represents an MFT record $STANDARD_INFORMATION attribute type
 */
typedef struct
{
    uint8_t creation_time;     /** 0x00 */
    uint8_t modified_time;     /** 0x08 */
    uint8_t mft_modified_time; /** 0x10 */
    uint8_t accessed_time;     /** 0x18 */
} standard_information;


/**
 * @brief Walks the attributes for a given MFT record.
 * @param record Pointer to the record with attributes to walk.
 * @param record_size Size of the record in bytes.
 * @param attribute_offset Offset to the first attribute.
 * @return true if the attribute list was walked, false otherwise.
 */
bool attributes_walk(
    const uint8_t *record,
    uint32_t record_size,
    uint16_t attribute_offset
);

#endif