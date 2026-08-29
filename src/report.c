#include <stdint.h>
#include <stdio.h>

#include "report.h"
#include "rules.h"
#include "timestamps.h"

#define RULE_MARKER "----"

/**
 * @brief Report filetime timestamp in UTC date time format.
 * @param label Pointer to the message to precede the output timestamp.
 * @param filetime uint64_t filetime timestamp to be converted to UTC.
 */
static void report_timestamp(const char *label, uint64_t filetime);

void report_record(const mft_record *record, uint32_t rule_flags)
{
    if (record == NULL || rule_flags == RULE_NONE)
    {
        return;
    }

    printf("\n");
    printf(SUCCESS_MARKER "SUSPICIOUS RECORD: %llu\n", (unsigned long long)record->record_number);

    wprintf(
        L"[+] Filename: %.*ls\n",
        record->metadata.fn.filename_length,
        (const wchar_t *)record->metadata.fn.filename
    );


    printf("\n");

    printf(SUCCESS_MARKER "$STANDARD_INFORMATION\n");
    report_timestamp("Created:", record->metadata.si.creation_time);
    report_timestamp("Modified:", record->metadata.si.modified_time);
    report_timestamp("MFT Modified:", record->metadata.si.mft_modified_time);
    report_timestamp("Accessed:", record->metadata.si.modified_time);

    printf("\n");

    printf(SUCCESS_MARKER "$FILE_NAME_INFORMATION\n");
    report_timestamp("Created:", record->metadata.fn.creation_time);
    report_timestamp("Modified:", record->metadata.fn.modified_time);
    report_timestamp("MFT Modified:", record->metadata.fn.mft_modified_time);
    report_timestamp("Accessed:", record->metadata.fn.modified_time);

    printf("\n");

    printf(SUCCESS_MARKER "DETECTION RULES TRIGGERED:\n");

    if (rule_flags & RULE_FLAG(RULE_SN_FN_MISMATCH))
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