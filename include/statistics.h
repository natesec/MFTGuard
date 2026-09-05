#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdint.h>
#include <stdbool.h>

#include "mft.h"
#include "attributes.h"

/** Temporary count attainted from clean sample. */
#define STATISTICS_MAX_FILE_NAMES 8

/**
 * @brief Represents aggregate statistical data.
 */
typedef struct
{
    uint64_t records_processed;
    uint64_t records_flagged;
    uint64_t records_unused;
    uint64_t records_reserved;
    uint64_t records_invalid;
    uint64_t records_skipped;

    uint64_t file_name_records;
    uint64_t multiple_file_name_records;
    uint64_t total_file_names;
    uint64_t max_file_names_per_record;

    uint64_t file_name_count_distribution[STATISTICS_MAX_FILE_NAMES + 1];

    uint64_t si_fn_mismatch_count;
    uint64_t timestamp_rollback_count;
    uint64_t zeroed_timestamp_count;
    uint64_t identical_timestamp_count;

    uint64_t creation_si_before_fn;
    uint64_t creation_si_equal_fn;
    uint64_t creation_si_after_fn;

    uint64_t modified_si_before_fn;
    uint64_t modified_si_equal_fn;
    uint64_t modified_si_after_fn;

    uint64_t mft_modified_si_before_fn;
    uint64_t mft_modified_si_equal_fn;
    uint64_t mft_modified_si_after_fn;

    uint64_t accessed_si_before_fn;
    uint64_t accessed_si_equal_fn;
    uint64_t accessed_si_after_fn;

    int64_t creation_delta_min;
    int64_t creation_delta_max;
    int64_t creation_delta_sum;
    uint64_t creation_delta_count;

    uint64_t creation_delta_over_1s;
    uint64_t creation_delta_over_1m;
    uint64_t creation_delta_over_1h;
    uint64_t creation_delta_over_1d;
    uint64_t creation_delta_over_1w;
    uint64_t creation_delta_over_30d;

    int64_t modified_delta_min;
    int64_t modified_delta_max;
    int64_t modified_delta_sum;
    uint64_t modified_delta_count;

    uint64_t modified_delta_over_1s;
    uint64_t modified_delta_over_1m;
    uint64_t modified_delta_over_1h;
    uint64_t modified_delta_over_1d;
    uint64_t modified_delta_over_1w;
    uint64_t modified_delta_over_30d;

    int64_t mft_modified_delta_min;
    int64_t mft_modified_delta_max;
    int64_t mft_modified_delta_sum;
    uint64_t mft_modified_delta_count;

    uint64_t mft_modified_delta_over_1s;
    uint64_t mft_modified_delta_over_1m;
    uint64_t mft_modified_delta_over_1h;
    uint64_t mft_modified_delta_over_1d;
    uint64_t mft_modified_delta_over_1w;
    uint64_t mft_modified_delta_over_30d;

    int64_t accessed_delta_min;
    int64_t accessed_delta_max;
    int64_t accessed_delta_sum;
    uint64_t accessed_delta_count;

    uint64_t accessed_delta_over_1s;
    uint64_t accessed_delta_over_1m;
    uint64_t accessed_delta_over_1h;
    uint64_t accessed_delta_over_1d;
    uint64_t accessed_delta_over_1w;
    uint64_t accessed_delta_over_30d;

    uint64_t modified_and_mft_modified_over_1d;
    uint64_t modified_and_mft_modified_over_1w;
    uint64_t modified_and_mft_modified_over_30d;
} statistics;

/**
 * @brief Initialize the statistics structure.
 * @param stats Pointer to the statistics struct to be initialized.
 */
void statistics_init(statistics *stats);

/**
 * @brief Collect statistically-relevant data for a single record.
 * @param stats Pointer to the statistics struct to store the data to.
 * @param metadata Pointer to the relevant record_metadata structure.
 * @param status The status returned after parsing the record.
 * @param rule_flags uin32_t value representing the rule bitmask / triggered rules.
 * @param should_report true if the record passed rules_should_report, false otherwise.
 */
void statistics_collect(
    statistics *stats,
    const record_metadata *metadata,
    mft_record_status status,
    uint32_t rule_flags,
    bool should_report
);

/**
 * @brief Print statistics to the console.
 * @param stats Pointer to the statistics struct to print from.
 */
void statistics_print(const statistics *stats);

#endif