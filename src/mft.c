#include "mft.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * Open file, determine file size, determine and allocate
 * one record buffer. Initialize state.
 */
bool mft_open(mft_file *mft, const char *path)
{
    if (mft == NULL || path == NULL)
    {
        fprintf(stderr, ERROR_MARKER "failed to open file");
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
        perror("fopen failed\n");
        return false;
    }

    if (!mft_get_file_size(mft))
    {
        return false;
    }

    if (!mft_get_record_size(mft))
    {
        return false;
    }

    return true;
}

/**
 * Read the current record.
 */
bool mft_read_record(mft_file *mft)
{
    return true;
}

/**
 * Interpret current buffer. Populate mft_record.
 */
bool mft_parse_record(const mft_file *mft, mft_record *record)
{
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
        fprintf(stderr, ERROR_MARKER "failed to get record size");
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
            fprintf(stderr, ERROR_MARKER "failed to read MFT record header");
        }

        return false;
    }

    if (memcmp(header, MFT_SIGNATURE_BAAD, 4) == 0)
    {
        fprintf(stderr, ERROR_MARKER "mft record signature is BAAD");
        return false;
    }
    else if (memcmp(header, MFT_SIGNATURE_FILE, 4) != 0)
    {
        fprintf(stderr, ERROR_MARKER "unrecognized/invalid MFT record signature");
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
        fprintf(stderr, ERROR_MARKER "allocated size field is zero in record header");
        return false;
    }

    mft->record_size = allocated_size;
    
    return true;
}