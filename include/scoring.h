#ifndef SCORING_H
#define SCORING_H

#include <stdint.h>

#include "candidate.h"
#include "statistics.h"
#include "rules.h"

/**
 * @brief Calculates the score contribution based on the candidate's rule flags.
 * @param rule_flags uint32_t bitmask containing the rule flags.
 * @param stats pointer to the statistics struct containing aggregate data.
 * @return uint32_t score contribution of the candidate's triggered rules.
 */
uint32_t scoring_score_rule_flags(uint32_t rule_flags, const statistics *stats);

#endif