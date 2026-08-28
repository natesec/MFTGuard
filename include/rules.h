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
    RULE_SN_FN_MISMATCH,
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

#endif