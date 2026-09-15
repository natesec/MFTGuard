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

/**
 * @brief Calculates the score contribution from timestamp clustering in the SI.
 * @param current pointer to the current candidate being scored.
 * @param candidates pointer to the candidates hash table.
 * @return uint32_t score contributing from timestamp clustering.
 */
uint32_t scoring_score_timestamp_clustering(const candidate *current, candidate *candidates);

/**
 * @brief Calculate the score contribution based on number of matching FILE_NAME
 *        parent directory fields.
 * @param current Pointer to the current candidate being scored.
 * @param candidates Pointer to the candidates hash table.
 * @return uint32_t score contributing from parent directory results.
 */
uint32_t scoring_score_parent_directory(const candidate *current, candidate *candidates);

#endif