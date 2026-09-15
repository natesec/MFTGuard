#ifndef CANDIDATE_H
#define CANDIDATE_H

#include <stdbool.h>
#include <stdint.h>

#include "attributes.h"
#include "uthash.h"

/**
 * @brief Represents a candidate record selected by rules_evaluate.
 */
typedef struct
{
    uint64_t record_number;
    uint32_t rule_flags;
    bool has_standard_information;
    standard_information si;
    file_name_information *file_names;
    uint32_t file_name_count;
    uint32_t confidence_score;
    UT_hash_handle hh;
} candidate;

/**
 * @brief Allocates a candidate for the hash table and copies record data.
 * @param candidates double pointer to the candidate hash table.
 * @param metadata pointer to the flagged record_metadata structure.
 * @param rule_flags uint32_t value containing the rule bitmask.
 * @return true if successful candidate creation, false otherwise.
 */
bool candidate_add(
    candidate **candidates,
    const record_metadata *metadata,
    uint32_t rule_flags
);

/**
 * @brief Frees all allocated candidates and their data.
 * @param candidates double pointer to the hash table.
 */
void candidate_free_all(candidate **candidates);

/**
 * @brief Calculate the confidence score for a single candidate.
 * @param current Pointer to the current candidate.
 * @param candidates Pointer to the candidates hash table.
 * @return uint32_t confidence score from the combined scoring function results.
 */
uint32_t candidate_calculate_score(const candidate *current, candidate *candidates);

#endif