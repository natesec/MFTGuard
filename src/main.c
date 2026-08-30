#include <stdio.h>
#include "mft.h"
#include "utils.h"
#include "rules.h"
#include "report.h"

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

    /** TESTING */
    
    mft.record_number = 625893;

    /** TESTING */

    if (!mft_read_record(&mft))
    {
        mft_close(&mft);
        return 1;
    }

    if (!mft_parse_record(&mft, &record, sector_size))
    {
        mft_close(&mft);
        return 1;
    }

    uint32_t rule_flags = rules_evaluate(&record.metadata);

    if (rule_flags != RULE_NONE)
    {
        report_record(&record, rule_flags);
    }

    mft_close(&mft);

    printf("\n\n");
    printf(SUCCESS_MARKER "MFT closed successfully\n");

    return 0;
}