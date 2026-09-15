#include "scoring.h"

#define TIMESTAMP_CLUSTER_WINDOW 10000000
#define TIMESTAMP_CLUSTER_SCORE 10

#define FILE_NAME_NAMESPACE_WIN32 0x01
#define FILE_NAME_NAMESPACE_WIN32_DOS 0x03

/**
 * @brief Determines if two timestamps are within a given time.
 * @param timestamp_a uint64_t first timestamp in FILETIME.
 * @param timestamp_b uint64_t second timestamp in FILETIME.
 * @return true if timestamps are near eachother, false otherwise.
 */
static bool scoring_timestamp_is_near(uint64_t timestamp_a, uint64_t timestamp_b);

/**
 * @brief Check if filename_namespace field is one of the accepted values.
 * @param fn Pointer to a candidate's FN struct.
 * @return true if the FN is usable, false otherwise.
 */
static bool scoring_is_usable_file_name(const file_name_information *fn);

/** TODO: implement statistics-based weighted, contextual score */
uint32_t scoring_score_rule_flags(uint32_t rule_flags, const statistics *stats)
{
    uint32_t score = 0;

    if (rule_flags & RULE_FLAG(RULE_SI_FN_MISMATCH))
    {
        score += 10;
    }

    if (rule_flags & RULE_FLAG(RULE_ZEROED_TIMESTAMP))
    {
        score += 20;
    }

    if (rule_flags & RULE_FLAG(RULE_IDENTICAL_TIMESTAMPS))
    {
        score += 15;
    }

    if (rule_flags & RULE_FLAG(RULE_TIMESTAMP_ROLLBACK))
    {
        score += 5;
    }

    return score;
}

uint32_t scoring_score_timestamp_clustering(const candidate *current, candidate *candidates)
{
    if (current == NULL || candidates == NULL)
    {
        return 0;
    }

    if (!current->has_standard_information)
    {
        return 0;
    }

    candidate *other;
    candidate *tmp;
    uint32_t matches = 0;

    HASH_ITER(hh, candidates, other, tmp)
    {
        if (other == current)
        {
            continue;
        }

        if (!other->has_standard_information)
        {
            continue;
        }

        if (
            scoring_timestamp_is_near(current->si.creation_time, other->si.creation_time) ||
            scoring_timestamp_is_near(current->si.modified_time, other->si.modified_time) ||
            scoring_timestamp_is_near(current->si.mft_modified_time, other->si.mft_modified_time) ||
            scoring_timestamp_is_near(current->si.accessed_time, other->si.accessed_time))
        {
            matches++;
        }
    }

    if (matches == 0)
    {
        return 0;
    }

    return TIMESTAMP_CLUSTER_SCORE;
}

uint32_t scoring_score_parent_directory(const candidate *current, candidate *candidates)
{
    candidate *other;
    candidate *temp;
    uint32_t matches = 0;

    if (current == NULL || candidates == NULL)
    {
        return 0;
    }

    HASH_ITER(hh, candidates, other, temp)
    {
        if (other == current)
        {
            continue;
        }

        bool parent_match = false;
        
        for (uint32_t i = 0; i < current->file_name_count && !parent_match; i++)
        {
            if (!scoring_is_usable_file_name(&current->file_names[i]))
            {
                continue;
            }

            for (uint32_t j = 0; j < other->file_name_count; j++)
            {
                if (!scoring_is_usable_file_name(&other->file_names[j]))
                {
                    continue;
                }

                if (current->file_names[i].parent_directory == other->file_names[j].parent_directory)
                {
                    parent_match = true;
                    break;
                }
            }
        }

        if (parent_match)
        {
            matches++;
        }
    }

    if (matches == 0)
    {
        return 0;
    }

    if (matches <= 2)
    {
        return 5;
    }

    if (matches <= 5)
    {
        return 10;
    }
    
    return 15;
}

static bool scoring_timestamp_is_near(uint64_t timestamp_a, uint64_t timestamp_b)
{
    uint64_t difference;

    if (timestamp_a >= timestamp_b)
    {
        difference = timestamp_a - timestamp_b;
    }
    else
    {
        difference = timestamp_b - timestamp_a;
    }

    return difference <= TIMESTAMP_CLUSTER_WINDOW;
}

static bool scoring_is_usable_file_name(const file_name_information *fn)
{
    if (fn == NULL)
    {
        return false;
    }

    return fn->filename_namespace == FILE_NAME_NAMESPACE_WIN32 ||
        fn->filename_namespace == FILE_NAME_NAMESPACE_WIN32_DOS;
}