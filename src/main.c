#include <stdio.h>
#include "mft.h"

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf(ERROR_MARKER "USAGE: %s <file>\n", argv[0]);
        return 1;
    }

    MftFile mft;

    printf(MESSAGE_MARKER "Opening $MFT file...\n");

    if (!mft_open(&mft, argv[1]))
    {
        printf(ERROR_MARKER "FAILED OPENING $MFT FILE.\n");
        return 1;
    }

    printf(MESSAGE_MARKER "File size: %llu bytes\n", (unsigned long long)mft.file_size);

    printf(MESSAGE_MARKER "Record size: %llu bytes\n", (unsigned long long)mft.record_size);

    printf(MESSAGE_MARKER "Record count: %llu\n", (unsigned long long)mft.record_count);

    mft_close(&mft);

    printf("[+] MFT closed successfully.\n");

    return 0;
}