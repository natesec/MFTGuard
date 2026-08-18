#ifndef FIXUP_H
#define FIXUP_H

#include <stdbool.h>
#include <stdint.h>

#define ERROR_MARKER "[!] "

/**
 * @brief Applies USA fixups to a record.
 * @param record Pointer to the record to apply the fixups to.
 * @param record_size Size of the record in bytes.
 * @param sector_size Size of a disk sector in bytes (512 for 1024-byte records).
 * @param usa_offset Offset to the USA table from the usa_offset field in the record header.
 * @param usa_count Number of entries in the USA.
 * @return true if fixups validated/applied successfully, false otherwise.
 */
bool fixup_apply(
    uint8_t *record, 
    uint32_t record_size, 
    uint32_t sector_size, 
    uint16_t usa_offset, 
    uint16_t usa_count
);

#endif