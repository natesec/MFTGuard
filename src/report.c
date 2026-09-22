#include <stdio.h>

#include "report.h"
#include "rules.h"
#include "timestamps.h"
#include "candidate.h"

#define RULE_MARKER "----"
#define REPORT_BANNER "-------------------------------------------------\n"

/**
 * @brief Report filetime timestamp in UTC date time format.
 * @param label Pointer to the message to precede the output timestamp.
 * @param filetime uint64_t filetime timestamp to be converted to UTC.
 */
static void report_timestamp(const char *label, uint64_t filetime);

/**
 * @brief Add record stats from the statistics struct to the overview object.
 * @param overview Pointer to the overview object.
 * @param stats Pointer to the populated statistics struct.
 * @return true if successfully added record stats, false otherwise.
 */
static bool report_add_record_statistics(cJSON *overview, const statistics *stats);

/**
 * @brief Add rule stats from the statistics struct to the overview object.
 * @param overview Pointer to the overview object.
 * @param stats Pointer to the populated statistics struct.
 * @return true if successfully added rule stats, false otherwise.
 */
static bool report_add_rule_statistics(cJSON *overview, const statistics *stats);

/**
 * @brief Add file name stats from the statistics struct to the overview object.
 * @param overview Pointer to the overview object.
 * @param stats Pointer to the populated statistics struct.
 * @return true if successfully added file name stats, false otherwise.
 */
static bool report_add_file_name_statistics(cJSON *overview, const statistics *stats);

/**
 * @brief Add SI FN timestamp stats from the statistics struct to the overview object.
 * @param overview Pointer to the overview object.
 * @param stats Pointer to the populated statistics struct.
 * @return true if successfully added SI/FN stats, false otherwise.
 */
static bool report_add_si_fn_statistics(cJSON *overview, const statistics *stats);

/**
 * @brief Add timestamp delta stats from the statistics struct to the overview object.
 * @param overview Pointer to the overview object.
 * @param stats Pointer to the populated statistics struct.
 * @return true if successfully added delta stats, false otherwise.
 */
static bool report_add_delta_statistics(cJSON *overview, const statistics *stats);

/**
 * @brief Write unformatted candidate information to the JSON report file.
 * @param output Pointer to an open FILE stream where the candidate will be written.
 * @param candidate Pointer to the populated candidate struct to be written.
 * @return true if candidate was printed to the file stream, false otherwise.
 */
static bool report_write_candidate(FILE *output, const candidate *candidate);

/**
 * @brief Add candidate SI fields to the SI cJSON object.
 * @param si Pointer to the SI object.
 * @param candidate Pointer to the populated candidate struct.
 * @return true if candidate SI fields successfully added, false otherwise.
 */
static bool report_add_candidate_si(cJSON *si, const candidate *candidate);

void report_record(const mft_record *record, uint32_t rule_flags)
{
    if (record == NULL || rule_flags == RULE_NONE)
    {
        return;
    }

    printf("\n" REPORT_BANNER);

    printf(SUCCESS_MARKER "SUSPICIOUS RECORD: %llu\n", (unsigned long long)record->record_number);

    printf("\n");

    printf(SUCCESS_MARKER "$STANDARD_INFORMATION\n");
    report_timestamp("Created:", record->metadata.si.creation_time);
    report_timestamp("Modified:", record->metadata.si.modified_time);
    report_timestamp("MFT Modified:", record->metadata.si.mft_modified_time);
    report_timestamp("Accessed:", record->metadata.si.accessed_time);

    printf("\n");

    if (record->metadata.file_name_count > 0)
    {
        printf(SUCCESS_MARKER "$FILE_NAME_INFORMATION (%u)\n", record->metadata.file_name_count);

        for (uint32_t i = 0; i < record->metadata.file_name_count; i++)
        {
            const file_name_information *fn = &record->metadata.file_names[i];

            if (!fn->is_usable)
            {
                continue;
            }

            printf(SUCCESS_MARKER "FILENAME #%u\n", i + 1);

            wprintf(
                L"[+] Filename: %.*ls\n",
                fn->filename_length,
                (const wchar_t *)fn->filename
            );

            report_timestamp("Created:", fn->creation_time);
            report_timestamp("Modified:", fn->modified_time);
            report_timestamp("MFT Modified:", fn->mft_modified_time);
            report_timestamp("Accessed:", fn->accessed_time);

            printf("\n");
        }
    }

    printf(SUCCESS_MARKER "DETECTION RULES TRIGGERED:\n");

    if (rule_flags & RULE_FLAG(RULE_SI_FN_MISMATCH))
    {
        printf(RULE_MARKER "SI/FN mismatch detected\n");
    }

    if (rule_flags & RULE_FLAG(RULE_TIMESTAMP_ROLLBACK))
    {
        printf(RULE_MARKER "Timestamp rollback detected\n");
    }

    if (rule_flags & RULE_FLAG(RULE_ZEROED_TIMESTAMP))
    {
        printf(RULE_MARKER "Zeroed timestamp precision detected\n");
    }

    if (rule_flags & RULE_FLAG(RULE_IDENTICAL_TIMESTAMPS))
    {
        printf(RULE_MARKER "Identical timestamps detected\n");
    }

    printf(REPORT_BANNER "\n");
}

bool report_initialize(report *report)
{
    if (report == NULL)
    {
        return false;
    }

    report->root = cJSON_CreateObject();

    if (report->root == NULL)
    {
        return false;
    }

    return true;
}

bool report_add_overview(report *report, const statistics *stats)
{
    if (report == NULL || report->root == NULL || stats == NULL)
    {
        return false;
    }

    cJSON *overview = cJSON_CreateObject();

    if (overview == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(report->root, "overview", overview);

    if (!report_add_record_statistics(overview, stats))
    {
        return false;
    }

    if (!report_add_rule_statistics(overview, stats))
    {
        return false;
    }

    if (!report_add_file_name_statistics(overview, stats))
    {
        return false;
    }

    if (!report_add_si_fn_statistics(overview, stats))
    {
        return false;
    }

    if (!report_add_delta_statistics(overview, stats))
    {
        return false;
    }

    return true;
}

void report_free(report *report)
{
    if (report == NULL)
    {
        return;
    }

    cJSON_Delete(report->root);
    report->root = NULL;
}

static void report_timestamp(const char *label, uint64_t filetime)
{
    struct tm utc_time;

    printf(
        "    %-15s 0x%016llX\n",
        label,
        (unsigned long long)filetime
    );

    if (filetime_to_utc(filetime, &utc_time))
    {
        printf(
            "    %-15s %04d-%02d-%02d %02d:%02d:%02d UTC\n",
            "",
            utc_time.tm_year + 1900,
            utc_time.tm_mon + 1,
            utc_time.tm_mday,
            utc_time.tm_hour,
            utc_time.tm_min,
            utc_time.tm_sec
        );
    }
    else
    {
        printf(
            "    %-15s INVALID FILETIME\n",
            ""
        );
    }
}

static bool report_add_record_statistics(cJSON *overview, const statistics *stats)
{
    if (overview == NULL || stats == NULL)
    {
        return false;
    }

    cJSON *records = cJSON_CreateObject();

    if (records == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(overview, "records", records);

    cJSON_AddItemToObject(records, "processed", cJSON_CreateNumber((double)stats->records_processed));
    cJSON_AddItemToObject(records, "flagged", cJSON_CreateNumber((double)stats->records_flagged));
    cJSON_AddItemToObject(records, "unused", cJSON_CreateNumber((double)stats->records_unused));
    cJSON_AddItemToObject(records, "reserved", cJSON_CreateNumber((double)stats->records_reserved));
    cJSON_AddItemToObject(records, "invalid", cJSON_CreateNumber((double)stats->records_invalid));
    cJSON_AddItemToObject(records, "skipped", cJSON_CreateNumber((double)stats->records_skipped));

    return true;
}

static bool report_add_rule_statistics(cJSON *overview, const statistics *stats)
{
    if (overview == NULL || stats == NULL)
    {
        return false;
    }

    cJSON *rules = cJSON_CreateObject();

    if (rules == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(overview, "rules", rules);

    cJSON_AddItemToObject(rules, "si_fn_mismatch", cJSON_CreateNumber((double)stats->si_fn_mismatch_count));
    cJSON_AddItemToObject(rules, "timestamp_rollback", cJSON_CreateNumber((double)stats->timestamp_rollback_count));
    cJSON_AddItemToObject(rules, "zeroed_timestamp", cJSON_CreateNumber((double)stats->zeroed_timestamp_count));
    cJSON_AddItemToObject(rules, "identical_timestamp", cJSON_CreateNumber((double)stats->identical_timestamp_count));

    return true;
}

static bool report_add_file_name_statistics(cJSON *overview, const statistics *stats)
{
    if (overview == NULL || stats == NULL)
    {
        return false;
    }

    cJSON *file_names = cJSON_CreateObject();

    if (file_names == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(overview, "file_names", file_names);

    cJSON_AddItemToObject(file_names, "records", cJSON_CreateNumber((double)stats->file_name_records));
    cJSON_AddItemToObject(file_names, "multiple_records", cJSON_CreateNumber((double)stats->multiple_file_name_records));
    cJSON_AddItemToObject(file_names, "total", cJSON_CreateNumber((double)stats->total_file_names));
    cJSON_AddItemToObject(file_names, "max_per_record", cJSON_CreateNumber((double)stats->max_file_names_per_record));

    cJSON *distribution = cJSON_CreateObject();

    if (distribution == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(file_names, "distribution", distribution);

    char key[32];

    for (uint32_t i = 0; i < STATISTICS_MAX_FILE_NAMES; i++)
    {
        snprintf(key, sizeof(key), "%u", i);
        cJSON_AddItemToObject(distribution, key, cJSON_CreateNumber((double)stats->file_name_count_distribution[i]));
    }

    return true;
}

static bool report_add_si_fn_statistics(cJSON *overview, const statistics *stats)
{
    if (overview == NULL || stats == NULL)
    {
        return false;
    }

    cJSON *si_fn_counts = cJSON_CreateObject();

    if (si_fn_counts == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(overview, "si_fn_counts", si_fn_counts);

    cJSON *creation = cJSON_CreateObject();
    cJSON *modified = cJSON_CreateObject();
    cJSON *mft_modified = cJSON_CreateObject();
    cJSON *accessed = cJSON_CreateObject();

    if (creation == NULL || modified == NULL || mft_modified == NULL || accessed == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(si_fn_counts, "creation", creation);
    cJSON_AddItemToObject(si_fn_counts, "modified", modified);
    cJSON_AddItemToObject(si_fn_counts, "mft_modified", mft_modified);
    cJSON_AddItemToObject(si_fn_counts, "accessed", accessed);

    cJSON_AddItemToObject(creation, "creation_si_before_fn", cJSON_CreateNumber((double)stats->creation_si_before_fn));
    cJSON_AddItemToObject(creation, "creation_si_equal_fn", cJSON_CreateNumber((double)stats->creation_si_equal_fn));
    cJSON_AddItemToObject(creation, "creation_si_after_fn", cJSON_CreateNumber((double)stats->creation_si_after_fn));

    cJSON_AddItemToObject(modified, "modified_si_before_fn", cJSON_CreateNumber((double)stats->modified_si_before_fn));
    cJSON_AddItemToObject(modified, "modified_si_equal_fn", cJSON_CreateNumber((double)stats->modified_si_equal_fn));
    cJSON_AddItemToObject(modified, "modified_si_after_fn", cJSON_CreateNumber((double)stats->modified_si_after_fn));

    cJSON_AddItemToObject(mft_modified, "mft_modified_si_before_fn", cJSON_CreateNumber((double)stats->mft_modified_si_before_fn));
    cJSON_AddItemToObject(mft_modified, "mft_modified_si_equal_fn", cJSON_CreateNumber((double)stats->mft_modified_si_equal_fn));
    cJSON_AddItemToObject(mft_modified, "mft_modified_si_after_fn", cJSON_CreateNumber((double)stats->mft_modified_si_after_fn));

    cJSON_AddItemToObject(accessed, "accessed_si_before_fn", cJSON_CreateNumber((double)stats->accessed_si_before_fn));
    cJSON_AddItemToObject(accessed, "accessed_si_equal_fn", cJSON_CreateNumber((double)stats->accessed_si_equal_fn));
    cJSON_AddItemToObject(accessed, "accessed_si_after_fn", cJSON_CreateNumber((double)stats->accessed_si_after_fn));

    return true;
}

static bool report_add_delta_statistics(cJSON *overview, const statistics *stats)
{
    if (overview == NULL || stats == NULL)
    {
        return false;
    }

    cJSON *timestamp_deltas = cJSON_CreateObject();

    if (timestamp_deltas == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(overview, "timestamp_deltas", timestamp_deltas);

    cJSON *creation = cJSON_CreateObject();
    cJSON *modified = cJSON_CreateObject();
    cJSON *mft_modified = cJSON_CreateObject();
    cJSON *accessed = cJSON_CreateObject();

    if (creation == NULL || modified == NULL || mft_modified == NULL || accessed == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(timestamp_deltas, "creation", creation);
    cJSON_AddItemToObject(timestamp_deltas, "modified", modified);
    cJSON_AddItemToObject(timestamp_deltas, "mft_modified", mft_modified);
    cJSON_AddItemToObject(timestamp_deltas, "accessed", accessed);

    cJSON_AddItemToObject(creation, "min", cJSON_CreateNumber((double)stats->creation_delta_min));
    cJSON_AddItemToObject(creation, "max", cJSON_CreateNumber((double)stats->creation_delta_max));
    cJSON_AddItemToObject(creation, "count", cJSON_CreateNumber((double)stats->creation_delta_count));
    cJSON_AddItemToObject(creation, "over_1s", cJSON_CreateNumber((double)stats->creation_delta_over_1s));
    cJSON_AddItemToObject(creation, "over_1m", cJSON_CreateNumber((double)stats->creation_delta_over_1m));
    cJSON_AddItemToObject(creation, "over_1h", cJSON_CreateNumber((double)stats->creation_delta_over_1h));
    cJSON_AddItemToObject(creation, "over_1d", cJSON_CreateNumber((double)stats->creation_delta_over_1d));
    cJSON_AddItemToObject(creation, "over_1w", cJSON_CreateNumber((double)stats->creation_delta_over_1w));
    cJSON_AddItemToObject(creation, "over_30d", cJSON_CreateNumber((double)stats->creation_delta_over_30d));

    cJSON_AddItemToObject(modified, "min", cJSON_CreateNumber((double)stats->modified_delta_min));
    cJSON_AddItemToObject(modified, "max", cJSON_CreateNumber((double)stats->modified_delta_max));
    cJSON_AddItemToObject(modified, "count", cJSON_CreateNumber((double)stats->modified_delta_count));
    cJSON_AddItemToObject(modified, "over_1s", cJSON_CreateNumber((double)stats->modified_delta_over_1s));
    cJSON_AddItemToObject(modified, "over_1m", cJSON_CreateNumber((double)stats->modified_delta_over_1m));
    cJSON_AddItemToObject(modified, "over_1h", cJSON_CreateNumber((double)stats->modified_delta_over_1h));
    cJSON_AddItemToObject(modified, "over_1d", cJSON_CreateNumber((double)stats->modified_delta_over_1d));
    cJSON_AddItemToObject(modified, "over_1w", cJSON_CreateNumber((double)stats->modified_delta_over_1w));
    cJSON_AddItemToObject(modified, "over_30d", cJSON_CreateNumber((double)stats->modified_delta_over_30d));

    cJSON_AddItemToObject(mft_modified, "min", cJSON_CreateNumber((double)stats->mft_modified_delta_min));
    cJSON_AddItemToObject(mft_modified, "max", cJSON_CreateNumber((double)stats->mft_modified_delta_max));
    cJSON_AddItemToObject(mft_modified, "count", cJSON_CreateNumber((double)stats->mft_modified_delta_count));
    cJSON_AddItemToObject(mft_modified, "over_1s", cJSON_CreateNumber((double)stats->mft_modified_delta_over_1s));
    cJSON_AddItemToObject(mft_modified, "over_1m", cJSON_CreateNumber((double)stats->mft_modified_delta_over_1m));
    cJSON_AddItemToObject(mft_modified, "over_1h", cJSON_CreateNumber((double)stats->mft_modified_delta_over_1h));
    cJSON_AddItemToObject(mft_modified, "over_1d", cJSON_CreateNumber((double)stats->mft_modified_delta_over_1d));
    cJSON_AddItemToObject(mft_modified, "over_1w", cJSON_CreateNumber((double)stats->mft_modified_delta_over_1w));
    cJSON_AddItemToObject(mft_modified, "over_30d", cJSON_CreateNumber((double)stats->mft_modified_delta_over_30d));

    cJSON_AddItemToObject(accessed, "min", cJSON_CreateNumber((double)stats->accessed_delta_min));
    cJSON_AddItemToObject(accessed, "max", cJSON_CreateNumber((double)stats->accessed_delta_max));
    cJSON_AddItemToObject(accessed, "count", cJSON_CreateNumber((double)stats->accessed_delta_count));
    cJSON_AddItemToObject(accessed, "over_1s", cJSON_CreateNumber((double)stats->accessed_delta_over_1s));
    cJSON_AddItemToObject(accessed, "over_1m", cJSON_CreateNumber((double)stats->accessed_delta_over_1m));
    cJSON_AddItemToObject(accessed, "over_1h", cJSON_CreateNumber((double)stats->accessed_delta_over_1h));
    cJSON_AddItemToObject(accessed, "over_1d", cJSON_CreateNumber((double)stats->accessed_delta_over_1d));
    cJSON_AddItemToObject(accessed, "over_1w", cJSON_CreateNumber((double)stats->accessed_delta_over_1w));
    cJSON_AddItemToObject(accessed, "over_30d", cJSON_CreateNumber((double)stats->accessed_delta_over_30d));

    return true;
}

static bool report_write_candidate(FILE *output, const candidate *candidate)
{
    if (output == NULL || candidate == NULL)
    {
        return false;
    }

    cJSON *object = cJSON_CreateObject();

    if (object == NULL)
    {
        return false;
    }

    /** CANDIDATE FIELD HELPER FUNCTIONS */

    if (candidate->has_standard_information)
    {
        cJSON *si = cJSON_CreateObject();

        if (si == NULL)
        {
            cJSON_Delete(object);
            return false;
        }

        if (!report_add_candidate_si(si, candidate))
        {
            cJSON_Delete(object);
            return false;
        }
    }

    /** /HELPER FUNCTIONS */

    char *json = cJSON_PrintUnformatted(object);

    if (json == NULL)
    {
        cJSON_Delete(object);
        return false;
    }

    if (fputs(json, output) == EOF)
    {
        free(json);
        cJSON_Delete(object);
        return false;
    }

    free(json);
    cJSON_Delete(object);

    return true;
}

static bool report_add_candidate_si(cJSON *si, const candidate *candidate)
{
    if (si == NULL || candidate == NULL)
    {
        return false;
    }

    struct tm tm_utc;
    char iso8601_str[25];

    /** TODO: include datetime AND FILETIME */
    cJSON_AddItemToObject(si, "creation_time", cJSON_CreateString(candidate->si.creation_time));

    if (!filetime_to_utc(candidate->si.creation_time, &tm_utc))
    {
        return false;
    }

    if (strftime(iso8601_str, sizeof(iso8601_str), "%Y-%m-%dT%H:%M:%SZ", &tm_utc) == 0)
    {
        return false;
    }
    
    cJSON_AddItemToObject(si, "creation_time_utc", cJSON_CreateString(iso8601_str));

    cJSON_AddItemToObject(si, "modified_time", cJSON_CreateString(candidate->si.modified_time));

    if (!filetime_to_utc(candidate->si.modified_time, &tm_utc))
    {
        return false;
    }

    if (strftime(iso8601_str, sizeof(iso8601_str), "%Y-%m-%dT%H:%M:%SZ", &tm_utc) == 0)
    {
        return false;
    }

    cJSON_AddItemToObject(si, "modified_time_utc", cJSON_CreateString(iso8601_str));

    cJSON_AddItemToObject(si, "mft_modified_time", cJSON_CreateString(candidate->si.mft_modified_time));

    if (!filetime_to_utc(candidate->si.mft_modified_time, &tm_utc))
    {
        return false;
    }

    if (strftime(iso8601_str, sizeof(iso8601_str), "%Y-%m-%dT%H:%M:%SZ", &tm_utc) == 0)
    {
        return false;
    }

    cJSON_AddItemToObject(si, "mft_modified_time_utc", cJSON_CreateString(iso8601_str));

    cJSON_AddItemToObject(si, "accessed_time", cJSON_CreateString(candidate->si.accessed_time));

    if (!filetime_to_utc(candidate->si.accessed_time, &tm_utc))
    {
        return false;
    }

    if (strftime(iso8601_str, sizeof(iso8601_str), "%Y-%m-%dT%H:%M:%SZ", &tm_utc) == 0)
    {
        return false;
    }

    cJSON_AddItemToObject(si, "accessed_time_utc", cJSON_CreateString(iso8601_str));

    return true;
}