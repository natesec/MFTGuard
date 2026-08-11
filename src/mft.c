#include "mft.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Open file, determine file size, determine and allocate
 *        one record buffer. Initialize state.
 */
bool mft_open(MftFile *mft, const char *path)
{
    if (mft == NULL || path == NULL)
    {
        return false;
    }

    mft->fptr = NULL;
    mft->buffer = NULL;
    mft->file_size = 0;
    mft->record_size = MFT_RECORD_SIZE;
    mft->record_number = 0;
    mft->record_count=0;

    mft->fptr = fopen(path, "rb");

    if (mft->fptr == NULL)
    {
        return false;
    }

    return true;
}

/**
 * @brief Read one record, advance record_number.
 */
bool mft_read_record(MftFile *mft)
{
    return true;
}

/**
 * @brief Interpret current buffer. Populate MftRecord.
 */
bool mft_parse_record(MftFile *mft, MftRecord *record)
{
    return true;
}

/**
 * @brief Close fd, free buffer, reset state.
 */
void mft_close(MftFile *mft)
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