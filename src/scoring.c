#include "scoring.h"

#define TIMESTAMP_CLUSTER_WINDOW 10000000
#define TIMESTAMP_CLUSTER_SCORE 10

/**
 * @brief Determines if two timestamps are within a given time.
 * @param timestamp_a uint64_t first timestamp in FILETIME.
 * @param timestamp_b uint64_t second timestamp in FILETIME.
 * @return true if timestamps are near eachother, false otherwise.
 */
static bool scoring_timestamp_is_near(uint64_t timestamp_a, uint64_t timestamp_b);

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