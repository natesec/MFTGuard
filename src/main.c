#include <stdio.h>
#include "mft.h"
#include "utils.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, ERROR_MARKER "USAGE: %s <file> <sector_size>\n", argv[0]);
        return 1;
    }

    mft_file mft;
    mft_record record;

    printf(MESSAGE_MARKER "Opening $MFT file...\n");

    const char *mft_path = argv[1];

    char *endptr = NULL;
    
    unsigned long sector_size_value = strtoul(argv[2], &endptr, 10);

    if (*argv[2] == '\0' || *endptr != '\0' || sector_size_value > UINT32_MAX)
    {
        fprintf(stderr, ERROR_MARKER "invalid sector size\n");
        return 1;
    }

    uint32_t sector_size = (uint32_t)sector_size_value;

    if (sector_size == 0)
    {
        fprintf(stderr, ERROR_MARKER "sector size cannot be zero");
        return 1;
    }

    if (!mft_open(&mft, mft_path))
    {
        fprintf(stderr, ERROR_MARKER "failed opening $MFT file");
        return 1;
    }

    printf("\n\n");
    printf(MESSAGE_MARKER "File size: %llu bytes\n", (unsigned long long)mft.file_size);
    printf(MESSAGE_MARKER "Record size: %llu bytes\n", (unsigned long long)mft.record_size);
    printf(MESSAGE_MARKER "Record count: %llu\n", (unsigned long long)mft.record_count);

    if (!mft_read_record(&mft))
    {
        mft_close(&mft);
        return 1;
    }

    printf("\n\n");
    printf(
        MESSAGE_MARKER "Signature: %02X %02X %02X %02X\n",
        mft.buffer[0], // 46
        mft.buffer[1], // 49
        mft.buffer[2], // 4C
        mft.buffer[3]  // 45
        );
    
    uint32_t be_signature = read_u32_le(mft.buffer);
    printf(MESSAGE_MARKER "Signature in big-endian: %08X\n", be_signature);

    if (!mft_parse_record(&mft, &record, sector_size))
    {
        mft_close(&mft);
        return 1;
    }

    printf("\n\n");
    printf(MESSAGE_MARKER "Record number: %llu\n", (unsigned long long)record.record_number);
    printf(MESSAGE_MARKER "Record header number: %u\n", record.header.record_number);
    printf(MESSAGE_MARKER "Used size: %u\n", record.header.used_size);
    printf(MESSAGE_MARKER "Allocated size: %u\n", record.header.allocated_size);
    printf(MESSAGE_MARKER "USA offset: %u\n", record.header.usa_offset);
    printf(MESSAGE_MARKER "USA count: %u\n", record.header.usa_count);
    printf(MESSAGE_MARKER "Attribute offset: %u\n", record.header.attribute_offset);

    mft_close(&mft);

    printf("\n\n");
    printf(SUCCESS_MARKER "MFT closed successfully\n");

    return 0;
}