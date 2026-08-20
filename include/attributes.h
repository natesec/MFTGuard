#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include <stdbool.h>
#include <stdint.h>

/** NTFS attribute type that marks the end of the attribute list. */
#define ATTRIBUTE_TYPE_END 0xFFFFFFFF

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