#include "statistics.h"

#include "rules.h"

#define STATISTICS_DELTA_SECOND 1000000000ULL
#define STATISTICS_DELTA_MINUTE 60000000000ULL
#define STATISTICS_DELTA_HOUR 3600000000000ULL
#define STATISTICS_DELTA_DAY 86400000000000ULL
#define STATISTICS_DELTA_WEEK 604800000000000ULL
#define STATISTICS_DELTA_MONTH 2629800000000000ULL

/**
 * @brief Update aggregate statistics for a timestamp delta.
 * @param delta calculated timestamp delta in 100-ns units.
 * @param min Pointer to the minimum delta.
 * @param max Pointer to the max delta.
 * @param sum Pointer to the cumulative delta sum.
 * @param count Pointer to the number of recorded deltas.
 */
static void statistics_update_delta(
    int64_t delta,
    int64_t *min,
    int64_t *max,
    int64_t *sum,
    uint64_t *count
);

/**
 * @brief Calculates the difference between an SI and an FN timestamp.
 * @param fn_timestamp FILETIME timestamp value from an FN attribute.
 * @param si_timestamp FILETIME timestamp value from an SI attribute.
 * @return uint64_t timestamp difference in 100-ns units.
 */
static int64_t statistics_timestamp_delta(
    uint64_t fn_timestamp,
    uint64_t si_timestamp
);

/**
 * @brief Print aggregate timestamp delta statistics.
 * @param label identifies timestamp being reported.
 * @param min minimum timestamp delta.
 * @param max maximum timestamp delta.
 * @param sum sum of all timestamp deltas.
 * @param count number of recorded deltas.
 */
static void statistics_print_delta(
    const char *label,
    int64_t min,
    int64_t max,
    int64_t sum,
    uint64_t count
);

/**
 * @brief Convert a signed delta value into a magnitude.
 * @param delta calculated by statistics_timestamp_delta.
 * @return uint64_t magnitude value from delta.
 */
static uint64_t statistics_delta_magnitude(int64_t delta);

void statistics_init(statistics *stats)
{
    if (stats == NULL)
    {
        return;
    }

    *stats = (statistics){0};
}

/**
 * @brief Update cummulative delta threshold values in the statistics struct.
 * @param magnitude delta magnitude returned by helper function.
 * @param over_1s Pointer to the over 1 second field in the stats struct.
 * @param over_1m Pointer to the over 1 min field in the stats struct.
 * @param over_1h Pointer to the over 1 hour field in the stats struct.
 * @param over_1d Pointer to the over 1 day field in the stats struct.
 * @param over_1w Pointer to the over 1 week field in the stats struct.
 * @param over_30d Pointer to the over 30 days field in the stats struct.
 */
static void statistics_update_delta_thresholds(
    uint64_t magnitude,
    uint64_t *over_1s,
    uint64_t *over_1m,
    uint64_t *over_1h,
    uint64_t *over_1d,
    uint64_t *over_1w,
    uint64_t *over_30d
);

/**
 * @brief Print the cummulative delta thresholds.
 * @param label Pointer to the label to be prepended to the print statement.
 * @param over_1s to the over 1 second field in the stats struct.
 * @param over_1m the over 1 min field in the stats struct.
 * @param over_1h the over 1 hour field in the stats struct.
 * @param over_1d the over 1 day field in the stats struct.
 * @param over_1w the over 1 week field in the stats struct.
 * @param over_30d the over 30 days field in the stats struct.
 */
static void statistics_print_delta_thresholds(
    const char *label,
    uint64_t over_1s,
    uint64_t over_1m,
    uint64_t over_1h,
    uint64_t over_1d,
    uint64_t over_1w,
    uint64_t over_30d
);

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

    uint32_t fn_count = metadata->file_name_count;

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


        /** ---------- CREATION TIME ---------- */
        int64_t creation_delta = statistics_timestamp_delta(
            fn->creation_time,
            metadata->si.creation_time
        );

        statistics_update_delta(
            creation_delta,
            &stats->creation_delta_min,
            &stats->creation_delta_max,
            &stats->creation_delta_sum,
            &stats->creation_delta_count
        );

        uint64_t creation_magnitude = statistics_delta_magnitude(creation_delta);

        statistics_update_delta_thresholds(
            creation_magnitude,
            &stats->creation_delta_over_1s,
            &stats->creation_delta_over_1m,
            &stats->creation_delta_over_1h,
            &stats->creation_delta_over_1d,
            &stats->creation_delta_over_1w,
            &stats->creation_delta_over_30d
        );

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

        /** ---------- MODIFIED ---------- */

        int64_t modified_delta = statistics_timestamp_delta(
            fn->modified_time,
            metadata->si.modified_time
        );

        statistics_update_delta(
            modified_delta,
            &stats->modified_delta_min,
            &stats->modified_delta_max,
            &stats->modified_delta_sum,
            &stats->modified_delta_count
        );

        uint64_t modified_magnitude = statistics_delta_magnitude(modified_delta);

        statistics_update_delta_thresholds(
            modified_magnitude,
            &stats->modified_delta_over_1s,
            &stats->modified_delta_over_1m,
            &stats->modified_delta_over_1h,
            &stats->modified_delta_over_1d,
            &stats->modified_delta_over_1w,
            &stats->modified_delta_over_30d
        );

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

        /** ---------- MFT MODIFIED ---------- */

        int64_t mft_modified_delta = statistics_timestamp_delta(
            fn->mft_modified_time,
            metadata->si.mft_modified_time
        );

        statistics_update_delta(
            mft_modified_delta,
            &stats->mft_modified_delta_min,
            &stats->mft_modified_delta_max,
            &stats->mft_modified_delta_sum,
            &stats->mft_modified_delta_count
        );

        uint64_t mft_modified_magnitude = statistics_delta_magnitude(mft_modified_delta);

        statistics_update_delta_thresholds(
            mft_modified_magnitude,
            &stats->mft_modified_delta_over_1s,
            &stats->mft_modified_delta_over_1m,
            &stats->mft_modified_delta_over_1h,
            &stats->mft_modified_delta_over_1d,
            &stats->mft_modified_delta_over_1w,
            &stats->mft_modified_delta_over_30d
        );

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

        /** ---------- ACCESSED ---------- */

        int64_t accessed_delta = statistics_timestamp_delta(
            fn->accessed_time,
            metadata->si.accessed_time
        );

        statistics_update_delta(
            accessed_delta,
            &stats->accessed_delta_min,
            &stats->accessed_delta_max,
            &stats->accessed_delta_sum,
            &stats->accessed_delta_count
        );

        uint64_t accessed_magnitude = statistics_delta_magnitude(accessed_delta);

        statistics_update_delta_thresholds(
            accessed_magnitude,
            &stats->accessed_delta_over_1s,
            &stats->accessed_delta_over_1m,
            &stats->accessed_delta_over_1h,
            &stats->accessed_delta_over_1d,
            &stats->accessed_delta_over_1w,
            &stats->accessed_delta_over_30d
        );

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

        /** ---------- COMBINATIONS ---------- */
        if (modified_magnitude > STATISTICS_DELTA_DAY &&
            mft_modified_magnitude > STATISTICS_DELTA_DAY)
        {
            stats->modified_and_mft_modified_over_1d++;
        }

        if (modified_magnitude > STATISTICS_DELTA_WEEK &&
            mft_modified_magnitude > STATISTICS_DELTA_WEEK)
        {
            stats->modified_and_mft_modified_over_1w++;
        }

        if (modified_magnitude > STATISTICS_DELTA_MONTH &&
            mft_modified_magnitude > STATISTICS_DELTA_MONTH)
        {
            stats->modified_and_mft_modified_over_30d++;
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

    printf("SI/FN TIMESTAMP DELTAS\n");
    statistics_print_delta(
        "Creation",
        stats->creation_delta_min,
        stats->creation_delta_max,
        stats->creation_delta_sum,
        stats->creation_delta_count
    );

    statistics_print_delta(
        "Modified",
        stats->modified_delta_min,
        stats->modified_delta_max,
        stats->modified_delta_sum,
        stats->modified_delta_count
    );

    statistics_print_delta(
        "MFT Modified",
        stats->mft_modified_delta_min,
        stats->mft_modified_delta_max,
        stats->mft_modified_delta_sum,
        stats->mft_modified_delta_count
    );

    statistics_print_delta(
        "Accessed",
        stats->accessed_delta_min,
        stats->accessed_delta_max,
        stats->accessed_delta_sum,
        stats->accessed_delta_count
    );

    printf("\n");

    printf("SI/FN ABSOLUTE TIMESTAMP DELTA MAGNITUDES\n");
    statistics_print_delta_thresholds(
    "Creation",
    stats->creation_delta_over_1s,
    stats->creation_delta_over_1m,
    stats->creation_delta_over_1h,
    stats->creation_delta_over_1d,
    stats->creation_delta_over_1w,
    stats->creation_delta_over_30d
    );

    statistics_print_delta_thresholds(
        "Modified",
        stats->modified_delta_over_1s,
        stats->modified_delta_over_1m,
        stats->modified_delta_over_1h,
        stats->modified_delta_over_1d,
        stats->modified_delta_over_1w,
        stats->modified_delta_over_30d
    );

    statistics_print_delta_thresholds(
    "MFT Modified",
    stats->mft_modified_delta_over_1s,
    stats->mft_modified_delta_over_1m,
    stats->mft_modified_delta_over_1h,
    stats->mft_modified_delta_over_1d,
    stats->mft_modified_delta_over_1w,
    stats->mft_modified_delta_over_30d
    );

    statistics_print_delta_thresholds(
    "Accessed",
    stats->accessed_delta_over_1s,
    stats->accessed_delta_over_1m,
    stats->accessed_delta_over_1h,
    stats->accessed_delta_over_1d,
    stats->accessed_delta_over_1w,
    stats->accessed_delta_over_30d
    );

    printf("\n");

    printf("Modified + MFT modified over 1d: %llu\n", stats->modified_and_mft_modified_over_1d);
    printf("Modified + MFT modified over 1w: %llu\n", stats->modified_and_mft_modified_over_1w);
    printf("Modified + MFT modified over 30d: %llu\n", stats->modified_and_mft_modified_over_30d);

    printf("\n");
}

static void statistics_update_delta(
    int64_t delta,
    int64_t *min,
    int64_t *max,
    int64_t *sum,
    uint64_t *count)
{
    if (*count == 0)
    {
        *min = delta;
        *max = delta;
    }
    else
    {
        if (delta < *min)
        {
            *min = delta;
        }

        if (delta > *max)
        {
            *max = delta;
        }
    }

    *sum += delta;
    (*count)++;
}

static int64_t statistics_timestamp_delta(
    uint64_t fn_timestamp,
    uint64_t si_timestamp)
{
    if (fn_timestamp >= si_timestamp)
    {
        return (int64_t)(fn_timestamp - si_timestamp);
    }

    return -(int64_t)(si_timestamp - fn_timestamp);
}

static void statistics_print_delta(
    const char *label,
    int64_t min,
    int64_t max,
    int64_t sum,
    uint64_t count)
{
    double mean = 0.0;

    if (count > 0)
    {
        mean = (double)sum / (long double)count;
    }

    printf(
        "%s: min=%lld, max=%lld, mean=%.2f (100-ns units)\n",
        label,
        (long long)min,
        (long long)max,
        mean
    );
}

static uint64_t statistics_delta_magnitude(int64_t delta)
{
    if (delta >= 0)
    {
        return (uint64_t)delta;
    }

    return (uint64_t)(-(delta + 1)) + 1;
}

static void statistics_update_delta_thresholds(
    uint64_t magnitude,
    uint64_t *over_1s,
    uint64_t *over_1m,
    uint64_t *over_1h,
    uint64_t *over_1d,
    uint64_t *over_1w,
    uint64_t *over_30d)
{
    if (magnitude >= STATISTICS_DELTA_SECOND)
    {
        (*over_1s)++;
    }

    if (magnitude >= STATISTICS_DELTA_MINUTE)
    {
        (*over_1m)++;
    }

    if (magnitude >= STATISTICS_DELTA_HOUR)
    {
        (*over_1h)++;
    }

    if (magnitude >= STATISTICS_DELTA_DAY)
    {
        (*over_1d)++;
    }

    if (magnitude >= STATISTICS_DELTA_WEEK)
    {
        (*over_1w)++;
    }

    if (magnitude >= STATISTICS_DELTA_MONTH)
    {
        (*over_30d)++;
    }
}

static void statistics_print_delta_thresholds(
    const char *label,
    uint64_t over_1s,
    uint64_t over_1m,
    uint64_t over_1h,
    uint64_t over_1d,
    uint64_t over_1w,
    uint64_t over_30d)
{
    printf(
        "%s: >1s=%llu, >1m=%llu, >1h=%llu, "
        ">1d=%llu, >7d=%llu, >30d=%llu\n",
        label,
        (unsigned long long)over_1s,
        (unsigned long long)over_1m,
        (unsigned long long)over_1h,
        (unsigned long long)over_1d,
        (unsigned long long)over_1w,
        (unsigned long long)over_30d
    );
}