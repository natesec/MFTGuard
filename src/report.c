#include <stdio.h>

#include "report.h"
#include "rules.h"
#include "timestamps.h"
#include "uthash.h"

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

/**
 * @brief Creates a JSON array for candidate FN data and adds the array to a cJSON object.
 * @param object Pointer to the cJSON object to add the array to.
 * @param candidate Pointer to the populated candidate struct.
 * @return true if successfully added all file_name_information struct values, false otherwise.
 */
static bool report_add_candidate_file_names(cJSON *object, const candidate *candidate);

/**
 * @brief Add both a FILETIME timestamp and converted UTC datetime to a cJSON object.
 * @param object Pointer to the cJSON object.
 * @param raw_name Pointer to the desired FILETIME timestamp name.
 * @param utc_name Pointer to the desired UTC datetime name.
 * @param filetime uint54_t filetime value to be added.
 * @return true if successfully added the FILETIME and UTC datetime to the object, false otherwise.
 */
static bool report_add_filetime(cJSON *object, const char *raw_name, const char *utc_name, uint64_t filetime);

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

bool report_write(const report *report, const candidate *candidates, const char *filename)
{
    if (report == NULL || report->root == NULL || filename == NULL)
    {
        return false;
    }

    FILE *output = fopen(filename, "w");

    if (output == NULL)
    {
        return false;
    }

    cJSON *overview = cJSON_GetObjectItemCaseSensitive(report->root, "overview");

    if (overview == NULL)
    {
        fclose(output);
        return false;
    }

    char *overview_json = cJSON_PrintUnformatted(overview);

    if (overview_json == NULL)
    {
        fclose(output);
        return false;
    }


    if (fputs("{\n", output) == EOF ||
        fputs("  \"overview\": ", output) == EOF ||
        fputs(overview_json, output) == EOF ||
        fputs(",\n", output) == EOF ||
        fputs("  \"candidates\": [\n", output) == EOF)
    {
        free(overview_json);
        fclose(output);
        return false;
    }

    free(overview_json);

    if (candidates != NULL)
    {
        candidate *current;
        candidate *tmp;
        bool first_candidate = true;

        /** Local cast to avoid const compiler warning */
        candidate *hash_candidates = (candidate *)candidates;

        HASH_ITER(hh, hash_candidates, current, tmp)
        {
            if (!first_candidate)
            {
                if (fputs(",\n", output) == EOF)
                {
                    fclose(output);
                    return false;
                }
            }

            if (!report_write_candidate(output, current))
            {
                fclose(output);
                return false;
            }

            first_candidate = false;
        }
    }

    if (fputs("\n  ]\n", output) == EOF ||
        fputs("}\n", output) == EOF)
    {
        fclose(output);
        return false;
    }

    if (fclose(output) != 0)
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

    cJSON_AddItemToObject(object, "record_number", cJSON_CreateNumber((double)candidate->record_number));
    cJSON_AddItemToObject(object, "rule_flags", cJSON_CreateNumber((double)candidate->rule_flags));
    cJSON_AddItemToObject(object, "has_standard_information", cJSON_CreateBool(candidate->has_standard_information));

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

        cJSON_AddItemToObject(object, "standard_information", si);
    }

    cJSON_AddItemToObject(object, "file_name_count", cJSON_CreateNumber((double)candidate->file_name_count));

    if (!report_add_candidate_file_names(object, candidate))
    {
        cJSON_Delete(object);
        return false;
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

    if (!report_add_filetime(si, "creation_time", "creation_time_utc", candidate->si.creation_time))
    {
        return false;
    }

    if (!report_add_filetime(si, "modified_time", "modified_time_utc", candidate->si.modified_time))
    {
        return false;
    }

    if (!report_add_filetime(si, "mft_modified_time", "mft_modified_time_utc", candidate->si.mft_modified_time))
    {
        return false;
    }

    if (!report_add_filetime(si, "accessed_time", "accessed_time_utc", candidate->si.accessed_time))
    {
        return false;
    }

    return true;
}

static bool report_add_candidate_file_names(cJSON *object, const candidate *candidate)
{
    if (object == NULL || candidate == NULL)
    {
        return false;
    }

    cJSON *file_names = cJSON_CreateArray();

    if (file_names == NULL)
    {
        return false;
    }

    for (uint32_t i = 0; i < candidate->file_name_count; i++)
    {
        cJSON *file_name = cJSON_CreateObject();

        if (file_name == NULL)
        {
            return false;
        }

        const file_name_information *fn = &candidate->file_names[i];

        cJSON_AddItemToObject(file_name, "parent_directory", cJSON_CreateNumber((double)fn->parent_directory));

        if (!report_add_filetime(file_name, "creation_time", "creation_time_utc", fn->creation_time))
        {
            cJSON_Delete(file_name);
            cJSON_Delete(file_names);
            return false;   
        }

        if (!report_add_filetime(file_name, "modified_time", "modified_time_utc", fn->modified_time))
        {
            cJSON_Delete(file_name);
            cJSON_Delete(file_names);
            return false;
        }

        if (!report_add_filetime(file_name, "mft_modified_time", "mft_modified_time_utc", fn->mft_modified_time))
        {
            cJSON_Delete(file_name);
            cJSON_Delete(file_names);
            return false;
        }

        if (!report_add_filetime(file_name, "accessed_time", "accessed_time_utc", fn->accessed_time))
        {
            cJSON_Delete(file_name);
            cJSON_Delete(file_names);
            return false;
        }

        cJSON_AddItemToObject(file_name, "allocated_size", cJSON_CreateNumber((double)fn->allocated_size));
        cJSON_AddItemToObject(file_name, "used_size", cJSON_CreateNumber((double)fn->used_size));
        cJSON_AddItemToObject(file_name, "flags", cJSON_CreateNumber((double)fn->flags));
        cJSON_AddItemToObject(file_name, "reparse_and_ea", cJSON_CreateNumber((double)fn->reparse_and_ea));
        cJSON_AddItemToObject(file_name, "filename_namespace", cJSON_CreateNumber((double)fn->filename_namespace));
        
        char *filename = malloc((size_t)fn->filename_length + 1);

        if (filename == NULL)
        {
            cJSON_Delete(file_name);
            cJSON_Delete(file_names);
            return false;
        }

        memcpy(filename, fn->filename, fn->filename_length);

        filename[fn->filename_length] = '\0';

        cJSON_AddItemToObject(file_name, "filename", cJSON_CreateString(filename));

        free(filename);

        cJSON_AddItemToArray(file_names, file_name);
    }

    cJSON_AddItemToObject(object, "file_names", file_names);

    return true;
}

static bool report_add_filetime(cJSON *object, const char *raw_name, const char *utc_name, uint64_t filetime)
{
    if (object == NULL || raw_name == NULL || utc_name == NULL)
    {
        return false;
    }

    struct tm tm_utc;
    char iso8601_str[25];
    char filetime_buffer[32];

    snprintf(filetime_buffer, sizeof(filetime_buffer), "%llu", (unsigned long long)filetime);

    cJSON *raw_time = cJSON_CreateString(filetime_buffer);

    if (raw_time == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(object, raw_name, raw_time);

    if (!filetime_to_utc(filetime, &tm_utc))
    {
        return false;
    }

    if (strftime(iso8601_str, sizeof(iso8601_str), "%Y-%m-%dT%H:%M:%SZ", &tm_utc) == 0)
    {
        return false;
    }

    cJSON *utc_time = cJSON_CreateString(iso8601_str);

    if (utc_time == NULL)
    {
        return false;
    }

    cJSON_AddItemToObject(object, utc_name, utc_time);

    return true;
}