#ifndef RULES_H
#define RULES_H

#include <stdbool.h>
#include <stdint.h>

#include "attributes.h"

/**
 * Used to bit shift rule IDs into respective positions marked
 * by the rule_id enum.
 */
#define RULE_FLAG(id) (1u << (id))

#define RULE_NONE 0u

/**
 * @brief Represents an ID for each detection rule.
 * @note Used in the form of a bit position. These flags
 *       will then undergo a bitwise OR operation to collapse
 *       all detections into a single variable.
 */
typedef enum
{
    RULE_SI_FN_MISMATCH,
    RULE_TIMESTAMP_ROLLBACK,
    RULE_ZEROED_TIMESTAMP,
    RULE_IDENTICAL_TIMESTAMPS,

    RULE_COUNT
} rule_id;

/**
 * @brief Evaluates all detection rules and returns the resulting rule flags.
 * @param metadata Pointer to the record_metadata structure containing parsed attributes.
 * @return uint32_t bitmask containing the results of all triggered detection rules.
 */
uint32_t rules_evaluate(const record_metadata *metadata);

/**
 * @brief Determines whether a combination of rule flags constitutes a record report.
 * @param rule_flags uint32_t rule flags bitmask containing all triggered detections rules.
 * @return true if the record should be reported, false otherwise.
 */
bool rules_should_report(uint32_t rule_flags);

/**
 * @brief Stores the count of each rule detected, disregarding further evaluation.
 * @param rule_flags uin32_t rule flags bitmask containing all triggered detection rules.
 * @param rule_counts Pointer to an array that stores the count for each rule fired.
 */
void rules_count(uint32_t rule_flags, uint64_t *rule_counts);

#endif