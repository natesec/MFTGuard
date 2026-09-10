#include "scoring.h"

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
}