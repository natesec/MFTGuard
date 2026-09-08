#ifndef CANDIDATE_H
#define CANDIDATE_H

#include <stdbool.h>
#include <stdint.h>

#include "attributes.h"
#include "uthash.h"

/**
 * @brief Represents a candidate selected by rules_evaluate.
 */
typedef struct
{
    uint64_t record_number;

    bool has_standard_information;
    standard_information si;

    file_name_information *file_names;
    uint32_t file_name_count;

    uint32_t confidence_score;

    UT_hash_handle hh;
} candidate;

#endif