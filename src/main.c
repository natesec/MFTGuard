#include <stdio.h>
#include <signal.h>

#include "mft.h"
#include "utils.h"
#include "rules.h"
#include "report.h"
#include "statistics.h"
#include "candidate.h"
#include "scoring.h"

static volatile sig_atomic_t shutdown_requested = 0;

/**
 * @brief Signal handler for unexpected shutdown.
 * @param signal int value of signal.
 */
static void handle_sigint(int signal);

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);

    if (argc < 3)
    {
        fprintf(stderr, ERROR_MARKER "USAGE: %s <file> <sector_size>\n", argv[0]);
        return 1;
    }

    mft_file mft;
    mft_record record = {0};

    printf(MESSAGE_MARKER "Opening $MFT file...\n");

    const char *mft_path = argv[1];

    char *endptr = NULL;
    
    unsigned long sector_size_value = strtoul(argv[2], &endptr, 10);

    if (*argv[2] == '\0' || *endptr != '\0' || sector_size_value > UINT32_MAX)
    {
        fprintf(stderr, ERROR_MARKER "invalid sector size\n");
        return 1;
    }

    uint32_t sector_size = (uint32_t)sector_size_value;

    if (sector_size == 0)
    {
        fprintf(stderr, ERROR_MARKER "sector size cannot be zero");
        return 1;
    }

    if (!mft_open(&mft, mft_path))
    {
        fprintf(stderr, ERROR_MARKER "failed opening $MFT file");
        return 1;
    }

    /** TESTING */
    //mft.record_number = 625893;
    uint64_t test_record_limit = 100;
    printf(
        MESSAGE_MARKER "Scanning %llu records...\n",
        (unsigned long long)mft.record_count
    );

    statistics stats;
    statistics_init(&stats);

    candidate *candidates = NULL;

    //while (mft.record_number < mft.record_count && mft.record_number < test_record_limit)
    while (mft.record_number < mft.record_count)
    {
        if (shutdown_requested)
        {
            printf(MESSAGE_MARKER"Shutdown requested. Stopping...\n");
            break;
        }

        if (!mft_read_record(&mft))
        {
            statistics_collect(&stats, NULL, MFT_RECORD_SKIPPED, RULE_NONE, false);
            mft.record_number++;
            continue;
        }

        mft_record_status status = mft_parse_record(&mft, &record, sector_size);

        if (status == MFT_RECORD_INVALID)
        {
            statistics_collect(&stats, NULL, status, RULE_NONE, false);
            mft.record_number++;
            continue;
        }

        if (status == MFT_RECORD_UNUSED)
        {
            statistics_collect(&stats, NULL, status, RULE_NONE, false);
            mft.record_number++;
            continue;
        }

        if (status == MFT_RECORD_RESERVED)
        {
            statistics_collect(&stats, NULL, status, RULE_NONE, false);
            mft.record_number++;
            continue;
        }


        uint32_t rule_flags = rules_evaluate(&record.metadata);
        bool should_report = rules_should_report(rule_flags);

        statistics_collect(&stats, &record.metadata, status, rule_flags, should_report);

        if (should_report)
        {
            //report_record(&record, rule_flags);

            if (!candidate_add(&candidates, &record.metadata, rule_flags))
            {
                fprintf(stderr, ERROR_MARKER "Failed to add candidate record");
                mft.record_number++;
                continue;
            }
        }

        mft.record_number++;
    }

    /** TESTING */

    candidate *current;
    candidate *tmp;

    scoring_statistics scoring_stats = {0};

    uint64_t candidate_count = 0;

    uint32_t cluster_matches;
    uint32_t parent_directory_matches;

    HASH_ITER(hh, candidates, current, tmp)
    {
        if (shutdown_requested)
        {
            printf(MESSAGE_MARKER"Shutdown requested. Stopping...\n");
            break;
        }

        candidate_count++;

        printf(
            MESSAGE_MARKER "Candidate number: %llu | Record number: %llu\n",
            (unsigned long long)candidate_count,
            (unsigned long long)current->record_number
        );

        uint32_t rules_score = scoring_score_rule_flags(current->rule_flags);
        current->confidence_score = rules_score;
        if (rules_score <= STATISTICS_RULE_SCORE_MAX)
        {
            scoring_stats.rule_score_distribution[rules_score]++;
        }

        uint32_t cluster_score = scoring_score_timestamp_clustering(current, candidates, &cluster_matches);
        current->confidence_score +=  cluster_score;
        if (cluster_score <= STATISTICS_CLUSTERING_SCORE_MAX)
        {
            scoring_stats.clustering_score_distribution[cluster_score]++;
        }

        if (cluster_matches >= STATISTICS_CLUSTERING_MATCH_MAX)
        {
            scoring_stats.clustering_match_distribution[STATISTICS_CLUSTERING_MATCH_MAX]++;
        }
        else
        {
            scoring_stats.clustering_match_distribution[cluster_matches]++;
        }

        uint32_t parent_directory_score = scoring_score_parent_directory(current, candidates, &parent_directory_matches);
        current->confidence_score += parent_directory_score;
        if (parent_directory_score <= STATISTICS_PARENT_DIRECTORY_SCORE_MAX)
        {
            scoring_stats.parent_directory_score_distribution[parent_directory_score]++;
        }

        if (parent_directory_matches >= STATISTICS_PARENT_DIRECTORY_MATCH_MAX)
        {
            scoring_stats.parent_directory_match_distribution[STATISTICS_PARENT_DIRECTORY_MATCH_MAX]++;
        }
        else
        {
            scoring_stats.parent_directory_match_distribution[parent_directory_matches]++;
        }

        if (current->confidence_score <= STATISTICS_TOTAL_SCORE_MAX)
        {
            scoring_stats.total_score_distribution[current->confidence_score]++;
        }

        /*
        printf("----Rules score: %u\n", rules_score);
        printf("----Clustering score: %u\n", cluster_score);
        printf("----Parent dir score: %u\n", parent_directory_score);
        printf("----Total Score: %u\n", current->confidence_score);
        cluster_score = 0;
        */
    }

    printf("CANDIDATE COUNT: %llu\n", (unsigned long long)candidate_count);

    printf(SUCCESS_MARKER "SCORING DISTRIBUTION\n");
    printf("----RULES\n");
    for (int32_t i = 0; i < STATISTICS_RULE_SCORE_MAX; i++)
    {
        if (scoring_stats.rule_score_distribution[i] > 0)
        {
            printf("--------Score %u: %llu\n", i, (unsigned long long)scoring_stats.rule_score_distribution[i]);
        }
    }

    printf("----CLUSTER SCORE\n");
    for (int i = 0; i < STATISTICS_CLUSTERING_SCORE_MAX; i++)
    {
        if (scoring_stats.clustering_score_distribution[i] > 0)
        {
            printf("--------Score: %u: %llu\n", i, (unsigned long long)scoring_stats.clustering_score_distribution[i]);
        }
    }

    printf("----PARENT DIR SCORE\n");
    for (int i = 0; i < STATISTICS_PARENT_DIRECTORY_SCORE_MAX; i++)
    {
        if (scoring_stats.parent_directory_score_distribution[i] > 0)
        {
            printf("--------Score: %u: %llu\n", i, (unsigned long long)scoring_stats.parent_directory_score_distribution[i]);
        }
    }

    printf("----TOTAL\n");
    for (int i = 0; i < STATISTICS_TOTAL_SCORE_MAX; i++)
    {
        if (scoring_stats.total_score_distribution[i] > 0)
        {
            printf("--------Score: %u: %llu\n", i, (unsigned long long)scoring_stats.total_score_distribution[i]);
        }
    }

    printf("----Cluster score match distribution\n");
    for (uint32_t i = 0; i < STATISTICS_CLUSTERING_MATCH_MAX; i++)
    {
        if (scoring_stats.clustering_match_distribution[i] > 0)
        {
            printf("--------Match %u: %llu\n", i, (unsigned long long)scoring_stats.clustering_match_distribution[i]);
        }
    }

    if (scoring_stats.clustering_match_distribution[STATISTICS_CLUSTERING_MATCH_MAX] > 0)
    {
        printf(
            "--------Match %u+: %llu\n",
            STATISTICS_CLUSTERING_MATCH_MAX,
            (unsigned long long)scoring_stats.clustering_match_distribution[STATISTICS_CLUSTERING_MATCH_MAX]
        );
    }

    printf("----Parent directory match distribution\n");
    for (uint32_t i = 0; i < STATISTICS_PARENT_DIRECTORY_MATCH_MAX; i++)
    {
        if (scoring_stats.parent_directory_match_distribution[i] > 0)
        {
            printf("--------Match %u: %llu\n", i, (unsigned long long)scoring_stats.parent_directory_match_distribution[i]);
        }
    }

    if (scoring_stats.parent_directory_match_distribution[STATISTICS_PARENT_DIRECTORY_MATCH_MAX] > 0)
    {
        printf(
            "--------Match %u+: %llu\n",
            STATISTICS_PARENT_DIRECTORY_MATCH_MAX,
            (unsigned long long)scoring_stats.parent_directory_match_distribution[STATISTICS_PARENT_DIRECTORY_MATCH_MAX]
        );
    }

    /** /TESTING */

    candidate_free_all(&candidates);

    statistics_print(&stats);

    mft_close(&mft);

    printf(SUCCESS_MARKER "MFT closed successfully\n");

    return 0;
}

static void handle_sigint(int signal)
{
    (void)signal;

    shutdown_requested = 1;
}