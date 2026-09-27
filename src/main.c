#include <stdio.h>
#include "mft.h"
#include "utils.h"
#include "rules.h"
#include "report.h"
#include "statistics.h"
#include "candidate.h"
#include "cli.h"

int main(int argc, char *argv[])
{
    mft_file mft;
    mft_record record = {0};

    printf(MESSAGE_MARKER "Opening $MFT file...\n");

    options cli_options;

    if (!parse_args(argc, argv, &cli_options))
    {
        fprintf(stderr, ERROR_MARKER "failed to parse cli arguments\n");
        print_help(argv[0]);
        return 1;
    }

    if (cli_options.help)
    {
        print_help(argv[0]);
        return  0;
    }

    const char *mft_path = cli_options.mft_path;
    const char *output_path = cli_options.output_path;
    uint32_t sector_size = cli_options.sector_size;

    if (!mft_open(&mft, mft_path))
    {
        fprintf(stderr, ERROR_MARKER "failed opening $MFT file");
        return 1;
    }

    printf(
        MESSAGE_MARKER "Scanning %llu records...\n",
        (unsigned long long)mft.record_count
    );

    statistics stats;
    statistics_init(&stats);

    candidate *candidates = NULL;

    while (mft.record_number < mft.record_count)
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
            if (!candidate_add(&candidates,&record.metadata, rule_flags))
            {
                fprintf(stderr, ERROR_MARKER "Failed to add candidate record");
                mft.record_number++;
                continue;
            }
        }

        mft.record_number++;
    }

    report report = {0};

    if (!report_initialize(&report))
    {
        fprintf(stderr, ERROR_MARKER "failed to initialize json\n");
        candidate_free_all(&candidates);
        mft_close(&mft);
        return 1;
    }

    if (!report_add_overview(&report, &stats))
    {
        fprintf(stderr, ERROR_MARKER "JSON: failed to add overview\n");
        report_free(&report);
        candidate_free_all(&candidates);
        mft_close(&mft);
        return 1;
    }

    
    printf(MESSAGE_MARKER "Generating JSON report\n");

    if (!report_write(&report, candidates, output_path))
    {
        fprintf(stderr, ERROR_MARKER "JSON: failed to write report\n");
        report_free(&report);
        candidate_free_all(&candidates);
        mft_close(&mft);
        return 1;
    }

    printf(SUCCESS_MARKER "JSON report complete\n");

    report_free(&report);
    candidate_free_all(&candidates);

    mft_close(&mft);

    printf(SUCCESS_MARKER "MFT closed successfully\n");

    return 0;
}