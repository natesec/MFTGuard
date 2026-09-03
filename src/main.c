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
    mft_record record = {0};

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
    //mft.record_number = 625893;
    uint64_t test_record_limit = 100000;
    printf(
        MESSAGE_MARKER "Scanning %llu records...\n",
        (unsigned long long)mft.record_count
    );

    uint64_t records_processed = 0;
    uint64_t records_unused = 0;
    uint64_t records_reserved = 0;
    uint64_t records_skipped = 0;
    uint64_t records_flagged = 0;

    uint64_t rule_counts[RULE_COUNT] = {0};

    /* while (mft.record_number < mft.record_count) */
    while (mft.record_number < mft.record_count && mft.record_number < test_record_limit)
    {
        if (!mft_read_record(&mft))
        {
            records_skipped++;
            mft.record_number++;
            continue;
        }

        mft_record_status status = mft_parse_record(&mft, &record, sector_size);

        if (status == MFT_RECORD_INVALID)
        {
            records_skipped++;
            mft.record_number++;
            continue;
        }

        if (status == MFT_RECORD_UNUSED)
        {
            records_unused++;
            mft.record_number++;
            continue;
        }

        if (status == MFT_RECORD_RESERVED)
        {
            records_reserved++;
            mft.record_number++;
            continue;
        }

        records_processed++;

        uint32_t rule_flags = rules_evaluate(&record.metadata);

        rules_count(rule_flags, rule_counts);

        if (rules_should_report(rule_flags))
        {
            records_flagged++;
            //report_record(&record, rule_flags);
        }

        mft.record_number++;
    }

    mft_close(&mft);

    printf(SUCCESS_MARKER "MFT closed successfully\n");

    return 0;
}