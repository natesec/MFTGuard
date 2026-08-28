#ifndef RULES_H
#define RULES_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Used to bit shift rule IDs into respective positions marked
 * by the rule_id enum.
 */
#define RULE_FLAG(id) (1u << (id))

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

#endif