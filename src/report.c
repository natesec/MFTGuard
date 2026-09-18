#include <stdio.h>

#include "report.h"
#include "rules.h"
#include "timestamps.h"

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