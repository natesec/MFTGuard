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

    for (uint32_t i = 0; i < metadata->file_name_count; i++)
    {
        const file_name_information *fn = &metadata->file_names[i];

        if (!fn->is_usable)
        {
            continue;
        }

        /** CREATION TIME */
        if (metadata->si.creation_time < fn->creation_time)
        {
            stats->creation_si_before_fn++;
        }
        else if (metadata->si.creation_time == fn->creation_time)
        {
            stats->creation_si_equal_fn++;
        }
        else
        {
            stats->creation_si_after_fn++;
        }

        /** MODIFIED */
        if (metadata->si.modified_time < fn->modified_time)
        {
            stats->modified_si_before_fn++;
        }
        else if (metadata->si.modified_time == fn->modified_time)
        {
            stats->modified_si_equal_fn++;
        }
        else
        {
            stats->modified_si_after_fn++;
        }

        /** MFT MODIFIED */
        if (metadata->si.mft_modified_time < fn->mft_modified_time)
        {
            stats->mft_modified_si_before_fn++;
        }
        else if (metadata->si.mft_modified_time == fn->mft_modified_time)
        {
            stats->mft_modified_si_equal_fn++;
        }
        else
        {
            stats->mft_modified_si_after_fn++;
        }

        /** ACCESSED */
        if (metadata->si.accessed_time < fn->accessed_time)
        {
            stats->accessed_si_before_fn++;
        }
        else if (metadata->si.accessed_time == fn->accessed_time)
        {
            stats->accessed_si_equal_fn++;
        }
        else
        {
            stats->accessed_si_after_fn++;
        }
    }
}

void statistics_print(const statistics *stats)
{
    printf("\n");

    printf("SCAN RESULTS\n");
    printf("Records processed: %llu\n", (unsigned long long)stats->records_processed);
    printf("Records flagged: %llu\n", (unsigned long long)stats->records_flagged);
    printf("Unused records: %llu\n", (unsigned long long)stats->records_unused);
    printf("Reserved Records: %llu\n", (unsigned long long)stats->records_reserved);
    printf("Invalid Records: %llu\n", (unsigned long long)stats->records_invalid);
    printf("Records skipped: %llu\n", (unsigned long long)stats->records_skipped);

    printf("\n");

    printf("FILE_NAME OVERVIEW\n");
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

    printf("DETECTION RESULTS\n");
    printf("SI/FN mismatches: %llu\n", (unsigned long long)stats->si_fn_mismatch_count);
    printf("Timestamp rollbacks: %llu\n", (unsigned long long)stats->timestamp_rollback_count);
    printf("Zeroed timestamps: %llu\n", (unsigned long long)stats->zeroed_timestamp_count);
    printf("Identical timestamps: %llu\n", (unsigned long long)stats->identical_timestamp_count);

    printf("\n");

    printf("SI/FN TIMESTAMP RELATIONSHIPS\n");
    printf(
        "Creation:    (SI < FN): %llu (SI == FN): %llu (SI > FN): %llu\n",
        (unsigned long long)stats->creation_si_before_fn,
        (unsigned long long)stats->creation_si_equal_fn,
        (unsigned long long)stats->creation_si_after_fn
    );

    printf(
        "Modified:    (SI < FN): %llu (SI == FN): %llu (SI > FN): %llu\n",
        (unsigned long long)stats->modified_si_before_fn,
        (unsigned long long)stats->modified_si_equal_fn,
        (unsigned long long)stats->modified_si_after_fn
    );

    printf(
        "MFT Modified:    (SI < FN): %llu (SI == FN): %llu (SI > FN): %llu\n",
        (unsigned long long)stats->mft_modified_si_before_fn,
        (unsigned long long)stats->mft_modified_si_equal_fn,
        (unsigned long long)stats->mft_modified_si_after_fn
    );

    printf(
        "Accessed:    (SI < FN): %llu (SI == FN): %llu (SI > FN): %llu\n",
        (unsigned long long)stats->accessed_si_before_fn,
        (unsigned long long)stats->accessed_si_equal_fn,
        (unsigned long long)stats->accessed_si_after_fn
    );

    printf("\n");

}