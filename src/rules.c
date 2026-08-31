#include <stdbool.h>
#include <stdint.h>

#include "rules.h"
//#include "attributes.h"
#include "timestamps.h"

#define SI_FN_MISMATCH_THRESHOLD 3
#define TIMESTAMP_ROLLBACK_THRESHOLD 2
#define ZEROED_TIMESTAMP_DIGITS 9
#define ZEROED_TIMESTAMP_MINIMUM_MATCHES 2

/**
 * @brief Detects if a variable number of timestamps in the $SI and $FN attributes don't match.
 * @param metadata Pointer to the record_metadata structure containing parsed attributes.
 * @return uint32_t number of mismatches detected.
 */
static uint32_t rule_si_fn_mismatch(const record_metadata *metadata);

/**
 * @brief Detects temporal-order violations such as accessed < created.
 * @param metadata Pointer to the record_metadata structure containing parsed attributes.
 * @return uint32_t number of rollbacks detected.
 */
static uint32_t rule_timestamp_rollback(const record_metadata *metadata);

/**
 * @brief Detects if multiple timestamps have a variable number of trailing zeroes.
 * @param metadata Pointer to the record_metadata structure containing parsed attributes.
 * @return true if set number of timestamps have the set number of trailing zeroes, false otherwise.
 */
static bool rule_zeroed_timestamp(const record_metadata *metadata);

/**
 * @brief Detects if all timestamps in $SI and $FN are identical.
 * @param metadata Pointer to the record_metadata structure containing parsed attributes.
 * @return true if all timestamps are identical, false otherwise.
 */
static bool rule_identical_timestamps(const record_metadata *metadata);

uint32_t rules_evaluate(const record_metadata *metadata)
{
    uint32_t rule_flags = 0;

    if (metadata == NULL)
    {
        return RULE_NONE;
    }

    uint32_t rule_si_fn_mismatch_count = rule_si_fn_mismatch(metadata);

    if (rule_si_fn_mismatch_count >= SI_FN_MISMATCH_THRESHOLD)
    {
        rule_flags |= RULE_FLAG(RULE_SN_FN_MISMATCH);
    }

    uint32_t rule_timestamp_rollback_count = rule_timestamp_rollback(metadata);

    if (rule_timestamp_rollback_count >= TIMESTAMP_ROLLBACK_THRESHOLD)
    {
        rule_flags |= RULE_FLAG(RULE_TIMESTAMP_ROLLBACK);
    }

    if (rule_zeroed_timestamp(metadata))
    {
        rule_flags |= RULE_FLAG(RULE_ZEROED_TIMESTAMP);
    }

    if (rule_identical_timestamps(metadata))
    {
        rule_flags |= RULE_FLAG(RULE_IDENTICAL_TIMESTAMPS);
    }

    return rule_flags; 
}

bool rules_should_report(uint32_t rule_flags)
{
    if (rule_flags == RULE_NONE)
    {
        return false;
    }

    if (rule_flags & RULE_FLAG(RULE_TIMESTAMP_ROLLBACK))
    {
        return true;
    }

    if ((rule_flags & RULE_FLAG(RULE_SN_FN_MISMATCH)) && (rule_flags & RULE_FLAG(RULE_ZEROED_TIMESTAMP)))
    {
        return true;
    }

    if ((rule_flags & RULE_FLAG(RULE_IDENTICAL_TIMESTAMPS)) && (rule_flags & RULE_FLAG(RULE_ZEROED_TIMESTAMP)))
    {
        return true;
    }

    if ((rule_flags & RULE_FLAG(RULE_SN_FN_MISMATCH)) && (rule_flags & RULE_FLAG(RULE_IDENTICAL_TIMESTAMPS)))
    {
        return true;
    }

    return false;
}

static uint32_t rule_si_fn_mismatch(const record_metadata *metadata)
{
    if (metadata == NULL ||
        !metadata->has_standard_information ||
        !metadata->has_file_name_information ||
        !metadata->has_usable_file_name_information)
    {
        return 0;
    }

    uint32_t mismatches = 0;

    if (!timestamp_equal(metadata->si.creation_time, metadata->fn.creation_time))
    {
        mismatches++;
    }

    if (!timestamp_equal(metadata->si.modified_time, metadata->fn.modified_time))
    {
        mismatches++;
    }

    if (!timestamp_equal(metadata->si.mft_modified_time, metadata->fn.mft_modified_time))
    {
        mismatches++;
    }

    if (!timestamp_equal(metadata->si.accessed_time, metadata->fn.accessed_time))
    {
        mismatches++;
    }

    return mismatches;
}

static uint32_t rule_timestamp_rollback(const record_metadata *metadata)
{
    if (metadata == NULL || !metadata->has_standard_information)
    {
        return 0;
    }

    uint32_t rollbacks = 0;

    if (metadata->si.modified_time < metadata->si.creation_time)
    {
        rollbacks++;
    }

    if (metadata->si.mft_modified_time < metadata->si.modified_time)
    {
        rollbacks++;
    }

    if (metadata->si.accessed_time < metadata->si.creation_time)
    {
        rollbacks++;
    }

    if (metadata->fn.modified_time < metadata->fn.creation_time)
    {
        rollbacks++;
    }

    return rollbacks;
}

static bool rule_zeroed_timestamp(const record_metadata *metadata)
{
    if (metadata == NULL || !metadata->has_standard_information)
    {
        return false;
    }

    uint32_t matches = 0;

    if (timestamp_low_digits_zeroed(metadata->si.creation_time, ZEROED_TIMESTAMP_DIGITS))
    {
        matches++;
    }

    if (timestamp_low_digits_zeroed(metadata->si.modified_time, ZEROED_TIMESTAMP_DIGITS))
    {
        matches++;
    }

    if (timestamp_low_digits_zeroed(metadata->si.mft_modified_time, ZEROED_TIMESTAMP_DIGITS))
    {
        matches++;
    }

    if (timestamp_low_digits_zeroed(metadata->si.accessed_time, ZEROED_TIMESTAMP_DIGITS))
    {
        matches++;
    }

    return matches >= ZEROED_TIMESTAMP_MINIMUM_MATCHES;
}

static bool rule_identical_timestamps(const record_metadata *metadata)
{
    if (metadata == NULL || !metadata->has_standard_information || !metadata->has_file_name_information)
    {
        return false;
    }

    return
        timestamp_equal(metadata->si.creation_time, metadata->si.modified_time) &&
        timestamp_equal(metadata->si.modified_time, metadata->si.mft_modified_time) &&
        timestamp_equal(metadata->si.mft_modified_time, metadata->si.accessed_time) &&

        timestamp_equal(metadata->fn.creation_time, metadata->si.creation_time) &&
        timestamp_equal(metadata->fn.modified_time, metadata->si.modified_time) &&
        timestamp_equal(metadata->fn.mft_modified_time, metadata->si.mft_modified_time) &&
        timestamp_equal(metadata->fn.accessed_time, metadata->si.accessed_time);
}