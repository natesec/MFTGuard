#ifndef TIMESTAMPS_H
#define TIMESTAMPS_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Check if two timestamps are equal.
 * @param first First timestamp to check against.
 * @param second Second timestamp to check against.
 * @return true if equal, false if unequal.
 */
bool timestamp_equal(uint64_t first, uint64_t second);

/**
 * @brief Check if a timestamp is before another timestamp.
 * @param first First timestamp to compare to.
 * @param second Second timestamp to compare to.
 * @return true if first is before the second, false otherwise.
 */
bool timestamp_is_before(uint64_t first, uint64_t second);

/**
 * @brief Check if a timestamp is after another timestamp.
 * @param first First timestamp to compare to.
 * @param second Second timestamp to compare to.
 * @return true if first is after the second, false otherwise.
 */
bool timestamp_is_after(uint64_t first, uint64_t second);

/**
 * @brief Check if the low digits of a timestamp are zeroed out.
 * @param timestamp The Timestamp to check.
 * @return true if the low digits are zeroed, false otherwise.
 */
bool timestamp_low_digits_zeroed(uint64_t timestamp);

#endif