#include "statistics.h"

#include "rules.h"

void statistics_init(statistics *stats)
{
    if (stats == NULL)
    {
        return;
    }

    *stats = (statistics){0};
}

void statistics_collect(statistics *stats,
    const record_metadata *metadata,
    mft_record_status status,
    uint32_t rule_flags,
    bool should_report)
{
    if (stats == NULL)
    {
        return;
    }

    switch (status)
    {
    case MFT_RECORD_USED:
        stats->records_processed++;
        break;
    
    case MFT_RECORD_UNUSED:
        stats->records_unused++;
        break;

    case MFT_RECORD_RESERVED:
        stats->records_reserved++;
        break;

    case MFT_RECORD_INVALID:
        stats->records_invalid++;
        break;

    case MFT_RECORD_SKIPPED:
        stats->records_skipped++;
        break;

    default:
        stats->records_skipped++;
        break;
    }

    if (status != MFT_RECORD_USED)
    {
        return;
    }

    uint64_t fn_count = metadata->file_name_count;

    if (fn_count > 0)
    {
        stats->file_name_records++;
        stats->total_file_names += fn_count;

        if (fn_count > 1)
        {
            stats->multiple_file_name_records++;
        }

        if (fn_count > stats->max_file_names_per_record)
        {
            stats->max_file_names_per_record = fn_count;
        }
    }

    if (fn_count <= STATISTICS_MAX_FILE_NAMES)
    {
        stats->file_name_count_distribution[fn_count]++;
    }
    else
    {
        stats->file_name_count_distribution[STATISTICS_MAX_FILE_NAMES + 1]++;
    }

    if (should_report)
    {
        stats->records_flagged++;
    }

    if (rule_flags & RULE_FLAG(RULE_SI_FN_MISMATCH))
    {
        stats->si_fn_mismatch_count++;
    }

    if (rule_flags & RULE_FLAG(RULE_TIMESTAMP_ROLLBACK))
    {
        stats->timestamp_rollback_count++;
    }

    if (rule_flags & RULE_FLAG(RULE_ZEROED_TIMESTAMP))
    {
        stats->zeroed_timestamp_count++;
    }

    if (rule_flags & RULE_FLAG(RULE_IDENTICAL_TIMESTAMPS))
    {
        stats->identical_timestamp_count++;
    }
}

void statistics_print(const statistics *stats)
{
    printf("\n");

    printf("Records processed: %llu\n", (unsigned long long)stats->records_processed);
    printf("Records flagged: %llu\n", (unsigned long long)stats->records_flagged);
    printf("Unused records: %llu\n", (unsigned long long)stats->records_unused);
    printf("Reserved Records: %llu\n", (unsigned long long)stats->records_reserved);
    printf("Invalid Records: %llu\n", (unsigned long long)stats->records_invalid);
    printf("Records skipped: %llu\n", (unsigned long long)stats->records_skipped);

    printf("\n");

    printf("Records with FILE_NAME attributes: %llu\n", (unsigned long long)stats->file_name_records);
    printf("Records with multiple FILE_NAME attributes: %llu\n", (unsigned long long)stats->multiple_file_name_records);
    printf("Total FILE_NAME attributes: %llu\n", (unsigned long long)stats->total_file_names);
    printf("Maximum FILE_NAME attributes in one record: %llu\n", (unsigned long long)stats->max_file_names_per_record);

    printf("\n");

    printf("FILE_NAME distribution\n");
    for (uint32_t i = 0; i <= STATISTICS_MAX_FILE_NAMES; i++)
    {
        printf(
            "%u FILE_NAME%s: %llu\n",
            i,
            (i == 1) ? "" : "s",
            (unsigned long long)stats->file_name_count_distribution[i]
        );
    }

    printf("\n");

    printf("SI/FN mismatches: %llu\n", (unsigned long long)stats->si_fn_mismatch_count);
    printf("Timestamp rollbacks: %llu\n", (unsigned long long)stats->timestamp_rollback_count);
    printf("Zeroed timestamps: %llu\n", (unsigned long long)stats->zeroed_timestamp_count);
    printf("Identical timestamps: %llu\n", (unsigned long long)stats->identical_timestamp_count);

    printf("\n");

}