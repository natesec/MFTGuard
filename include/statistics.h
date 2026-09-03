#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdint.h>
#include <stdbool.h>

#include "mft.h"
#include "attributes.h"

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

    uint64_t si_fn_mismatch_count;
    uint64_t timestamp_rollback_count;
    uint64_t zeroed_timestamp_count;
    uint64_t identical_timestamp_count;
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