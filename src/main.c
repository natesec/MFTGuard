#include <stdio.h>
#include "mft.h"
#include "utils.h"
#include "rules.h"
#include "report.h"
#include "statistics.h"
#include "candidate.h"

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
    uint64_t test_record_limit = 100;
    printf(
        MESSAGE_MARKER "Scanning %llu records...\n",
        (unsigned long long)mft.record_count
    );

    statistics stats;
    statistics_init(&stats);

    candidate *candidates = NULL;

    //while (mft.record_number < mft.record_count)
    while (mft.record_number < mft.record_count && mft.record_number < test_record_limit)
    {
        if (!mft_read_record(&mft))
        {
            statistics_collect(&stats, NULL, MFT_RECORD_SKIPPED, RULE_NONE, false);
            mft.record_number++;
            continue;
        }

        mft_record_status status = mft_parse_record(&mft, &record, sector_size);

        if (status == MFT_RECORD_INVALID)
        {
            statistics_collect(&stats, NULL, status, RULE_NONE, false);
            mft.record_number++;
            continue;
        }

        if (status == MFT_RECORD_UNUSED)
        {
            statistics_collect(&stats, NULL, status, RULE_NONE, false);
            mft.record_number++;
            continue;
        }

        if (status == MFT_RECORD_RESERVED)
        {
            statistics_collect(&stats, NULL, status, RULE_NONE, false);
            mft.record_number++;
            continue;
        }


        uint32_t rule_flags = rules_evaluate(&record.metadata);
        bool should_report = rules_should_report(rule_flags);

        statistics_collect(&stats, &record.metadata, status, rule_flags, should_report);

        if (should_report)
        {
            //report_record(&record, rule_flags);

            if (!candidate_add(&candidates,&record.metadata, rule_flags))
            {
                fprintf(stderr, ERROR_MARKER "Failed to add candidate record");
                mft.record_number++;
                continue;
            }
        }

        mft.record_number++;
    }

    /** TESTING */

    candidate *current;
    candidate *tmp;

    HASH_ITER(hh, candidates, current, tmp)
    {
        printf(MESSAGE_MARKER "Candidate record: %llu", (unsigned long long)current->record_number);
        for (uint32_t i = 0; i < current->file_name_count; i++)
        {
            wprintf(
                L"----Filename: %.*ls\n",
                current->file_names[i].filename_length,
                (const wchar_t *)current->file_names[i].filename
            );
        }
    }

    /** /TESTING */

    candidate_free_all(&candidates);

    //statistics_print(&stats);

    mft_close(&mft);

    printf(SUCCESS_MARKER "MFT closed successfully\n");

    return 0;
}