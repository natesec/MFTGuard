#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mft.h"
#include "fixup.h"
#include "attributes.h"
/** Little-endian conversion */
#include "utils.h"

/**
 * @brief Calculate file size of the $MFT file.
 * @param mft Pointer to the declared mft_file structure.
 * @return true if able to get the file size, false otherwise.
 */
static bool mft_get_file_size(mft_file *mft);

/**
 * @brief Get the allocated size field from the MFT record header.
 * @param mft Pointer to the declared mft_file structure.
 * @return true if able to get the record size, false otherwise.
 */
static bool mft_get_record_size(mft_file *mft);

/**
 * @brief Validate the record size obtained from the allocated
 *        size field of the MFT record header.
 * @param mft Pointer to the declared mft_file structure.
 * @return true if record size is correct, false otherwise.
 */
static bool mft_validate_record_size(const mft_file *mft);

/**
 * @brief Validate the record header fields to ensure the record
 *        is viable for parsing.
 * @param mft Pointer to the declared mft_file structure.
 * @param true if the record header is valid, false otherwise.
 */
static bool mft_validate_record_header(const mft_file *mft, const mft_record *record);

/**
 * Open file, determine file size, determine and allocate
 * one record buffer. Initialize state.
 */
bool mft_open(mft_file *mft, const char *path)
{
    if (mft == NULL || path == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to open file\n");
        return false;
    }

    mft->fptr = NULL;
    mft->buffer = NULL;
    mft->file_size = 0;
    mft->record_size = MFT_DEFAULT_RECORD_SIZE;
    mft->record_number = 0;
    mft->record_count=0;

    mft->fptr = fopen(path, "rb");

    if (mft->fptr == NULL)
    {
        perror("fopen failed");
        return false;
    }

    if (!mft_get_file_size(mft) || !mft_get_record_size(mft) || !mft_validate_record_size(mft))
    {
        return false;
    }

    mft->record_count = mft->file_size / mft->record_size;

    mft->buffer = malloc(mft->record_size);
    if (mft->buffer == NULL)
    {
        perror("malloc failed");
        return false;
    }

    return true;
}

/**
 * Read the current record into the buffer.
 */
bool mft_read_record(mft_file *mft)
{
    if (mft == NULL || mft->fptr == NULL || mft->buffer == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to read record\n");
        return false;
    }

    if (mft->record_number > mft->record_count)
    {
        fprintf(stderr, ERROR_MARKER "record number out of range\n");
        return false;
    }

    uint64_t offset = mft->record_number * mft->record_size;

    if (_fseeki64(mft->fptr, offset, SEEK_SET) != 0)
    {
        perror("_fseeki64 failed");
        return false;
    }

    size_t record_bytes_read = fread(mft->buffer, 1, (size_t)mft->record_size, mft->fptr);

    if (record_bytes_read != (size_t)mft->record_size)
    {
        if (ferror(mft->fptr))
        {
            perror("fread failed");
        }
        else
        {
            fprintf(stderr, ERROR_MARKER "failed to read record bytes");
        }

        return false;
    }

    return true;
}

/**
 * Interpret current buffer. Populate mft_record.
 */
bool mft_parse_record(mft_file *mft, mft_record *record, uint32_t sector_size)
{
    if (mft == NULL || record == NULL || mft->buffer == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to parse MFT record\n");
        return false;
    }

    // Size of the record header = 0x30 / 48 bytes.
    if (mft->record_size < 0x30)
    {
        fprintf(stderr, ERROR_MARKER "record is too small\n");
        return false;
    }

    memcpy(record->header.signature, mft->buffer + 0x0, 4);

    record->header.usa_offset = read_u16_le(mft->buffer + 0x04);
    record->header.usa_count = read_u16_le(mft->buffer + 0x06);
    record->header.lsn = read_u64_le(mft->buffer + 0x08);
    record->header.sequence_number = read_u16_le(mft->buffer + 0x10);
    record->header.hard_link_count = read_u16_le(mft->buffer + 0x12);
    record->header.attribute_offset = read_u16_le(mft->buffer + 0x14);
    record->header.flags = read_u16_le(mft->buffer + 0x16);
    record->header.used_size = read_u32_le(mft->buffer + 0x18);
    record->header.allocated_size = read_u32_le(mft->buffer + 0x1C);
    record->header.base_record = read_u64_le(mft->buffer + 0x20);
    record->header.next_attribute_id = read_u16_le(mft->buffer + 0x28);
    record->header.alignment = read_u16_le(mft->buffer + 0x2A);
    record->header.record_number = read_u32_le(mft->buffer + 0x2C);

    record->record_number = mft->record_number;

    /** TODO: apply/validate fixups, walk attributes, timestamp analysis */
    if (!mft_validate_record_header(mft, record))
    {
        return false;
    }

    if (!fixup_apply(
        mft->buffer,
        (uint32_t)mft->record_size,
        sector_size,
        record->header.usa_offset,
        record->header.usa_count))
    {
        /** TODO: Store record number for report if USN mismatches (use enum in fixup.c/h) */
        return false;
    }

    record->metadata.record_number= record->record_number;

    if (!attributes_walk(
        mft->buffer,
        (uint32_t)mft->record_size,
        record->header.attribute_offset,
        &record->metadata))
    {
        return false;
    }

    /** TESTING */
    
    printf("\n\n[?] record_metadata");

    if (record->metadata.has_standard_information)
    {
        printf(MESSAGE_MARKER "SI creation: 0x%016llx\n",
        (unsigned long long)record->metadata.si.creation_time);
    }

    if (record->metadata.has_file_name_information)
    {
        printf(MESSAGE_MARKER "FN creation: 0x%016llx\n",
        (unsigned long long)record->metadata.fn.creation_time);
    }

    /** TESTING */

    return true;
}

/**
 * Close fd, free buffer, reset state.
 */
void mft_close(mft_file *mft)
{
    if (mft == NULL)
    {
        return;
    }

    if (mft->fptr != NULL)
    {
        fclose(mft->fptr);
        mft->fptr = NULL;
    }

    if (mft->buffer != NULL)
    {
        free(mft->buffer);
        mft->buffer = NULL;
    }

    mft->file_size = 0;
    mft->record_size = 0;
    mft->record_number = 0;
    mft->record_count = 0;
}

static bool mft_get_file_size(mft_file *mft)
{
    // Move to end of file and get position to determine size
    if (_fseeki64(mft->fptr, 0, SEEK_END) != 0)
    {
        perror("_fseeki64 failed");
    }

    int64_t size = _ftelli64(mft->fptr);

    if (size == -1L)
    {
        perror("_ftelli64 failed");
        return false;
    }

    mft->file_size = (uint64_t)size;

    return true;
}

/**
 * Gets allocated size field from buffer using the offset from the FILE signature
 * read first 32 bytes -> verify FILE -> read allocated size field -> set record_size
 */
static bool mft_get_record_size(mft_file *mft)
{
    if (mft == NULL || mft->fptr == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to get record size\n");
        return false;
    }

    // Allocated size field ends at 0x20
    uint8_t header[MFT_RECORD_ALLOCATED_SIZE_END];

    if (_fseeki64(mft->fptr, 0, SEEK_SET) != 0)
    {
        perror("_fseeki64 failed");
        return false;
    }

    size_t bytes_read = fread(
        header,
        1,
        sizeof(header),
        mft->fptr
    );

    if (bytes_read != sizeof(header))
    {
        if (ferror(mft->fptr))
        {
            perror("fread failed");
        }
        else
        {
            fprintf(stderr, ERROR_MARKER "failed to read MFT record header\n");
        }

        return false;
    }

    if (memcmp(header, MFT_SIGNATURE_BAAD, 4) == 0)
    {
        /** TODO: add feature to make note of corrupt record and then skip */
        fprintf(stderr, ERROR_MARKER "mft record signature is BAAD\n");
        return false;
    }
    else if (memcmp(header, MFT_SIGNATURE_FILE, 4) != 0)
    {
        fprintf(stderr, ERROR_MARKER "unrecognized/invalid MFT record signature\n");
        return false;
    }

    uint32_t allocated_size;

    memcpy(
        &allocated_size,
        header + MFT_RECORD_ALLOCATED_SIZE_OFFSET,
        sizeof(allocated_size)
    );

    if (allocated_size == 0)
    {
        fprintf(stderr, ERROR_MARKER "allocated size field is zero in record header\n");
        return false;
    }

    mft->record_size = allocated_size;
    
    return true;
}

static bool mft_validate_record_size(const mft_file *mft)
{
    if (mft == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to validate record size\n");
        return false;
    }

    if (mft->record_size == 0)
    {
        fprintf(stderr, ERROR_MARKER "invalid record size of 0\n");
        return false;
    }

    if (mft->record_size != MFT_DEFAULT_RECORD_SIZE)
    {
        fprintf(
            stderr,
            ERROR_MARKER "non-standard record size %llu\n",
            (unsigned long long)(mft->record_size)
        );
        return false;
    }

    if (mft->record_size > mft->file_size)
    {
        fprintf(stderr, ERROR_MARKER "record size exceeds file size\n");
        return false;
    }

    if (mft->file_size % mft->record_size != 0)
    {
        fprintf(stderr, ERROR_MARKER "file size is not evenly divisible by record size\n");
        return false;
    }

    return true;
}

/** 
 * USA offset, attribute offset within record, USA count within bounds, used size
 * does not exceed allocated size or record size, record header < record size
 */
static bool mft_validate_record_header(const mft_file *mft, const mft_record *record)
{
    // consider moving signature checks here
    const mft_record_header *header = &record->header;

    if (header->usa_offset >= mft->record_size)
    {
        fprintf(stderr, ERROR_MARKER "invalid record header: usa offset > record size");
        return false;
    }

    if (header->usa_count == 0)
    {
        fprintf(stderr, ERROR_MARKER "invalid record header: usa count = 0");
        return false;
    }

    if (header->attribute_offset >= mft->record_size)
    {
        fprintf(stderr, ERROR_MARKER "invalid record header: attr offset > record size");
        return false;
    }

    if (header->used_size > header->allocated_size)
    {
        fprintf(stderr, ERROR_MARKER "invalid record header: used size > allocated size");
        return false;
    }

    if (header->used_size > mft->record_size)
    {
        fprintf(stderr, ERROR_MARKER "invalid record header: used size > record size");
        return false;
    }

    return true;
}
