#include <stdbool.h>
#include <stdint.h>

#include "rules.h"
//#include "attributes.h"
#include "timestamps.h"

#define ZEROED_TIMESTAMP_DIGITS 9
#define ZEROED_TIMESTAMP_MINIMUM_MATCHES 2

/**
 * @brief Detects if multiple timestamps have a variable number of trailing zeroes.
 * @param metadata Pointer to the record_metadata structure containing parsed attributes.
 * @return true if set number of timestamps have the set number of trailing zeroes, false otherwise.
 */
static bool rule_zeroed_timestamp(const record_metadata *metadata);

/**
 * @brief Detects if all timestamps are identical.
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
    if (metadata == NULL || !metadata->has_standard_information)
    {
        return false;
    }

    return
        timestamp_equal(metadata->si.creation_time, metadata->si.modified_time) &&
        timestamp_equal(metadata->si.modified_time, metadata->si.mft_modified_time) &&
        timestamp_equal(metadata->si.mft_modified_time, metadata->si.accessed_time);
}