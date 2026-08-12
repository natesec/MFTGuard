#include <stdio.h>
#include "mft.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        fprintf(stderr, ERROR_MARKER "USAGE: %s <file>\n", argv[0]);
        return 1;
    }

    mft_file mft;

    printf(MESSAGE_MARKER "Opening $MFT file...\n");

    if (!mft_open(&mft, argv[1]))
    {
        fprintf(stderr, ERROR_MARKER "Failed opening $MFT file");
        return 1;
    }

    printf(MESSAGE_MARKER "File size: %llu bytes\n", (unsigned long long)mft.file_size);

    printf(MESSAGE_MARKER "Record size: %llu bytes\n", (unsigned long long)mft.record_size);

    printf(MESSAGE_MARKER "Record count: %llu\n", (unsigned long long)mft.record_count);

    mft_close(&mft);

    printf(SUCCESS_MARKER "MFT closed successfully\n");

    return 0;
}