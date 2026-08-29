#include <stdint.h>
#include <stdio.h>

#include "report.h"
#include "rules.h"
#include "timestamps.h"

#define RULE_MARKER "----"

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


    printf("\n$STANDARD_INFORMATION\n");
    
    printf("    %-15s 0x%016llX\n",
        "Created:",
        (unsigned long long)record->metadata.si.creation_time);
    
    printf("    %-15s 0x%016llX\n",
        "Modified:",
        (unsigned long long)record->metadata.si.modified_time);
    
    printf("    %-15s 0x%016llX\n",
        "MFT Modified:",
        (unsigned long long)record->metadata.si.mft_modified_time);

    printf("    %-15s 0x%016llX\n",
        "Accessed:",
        (unsigned long long)record->metadata.si.accessed_time);


    printf("\n$FILE_NAME_INFORMATION\n");

    printf("    %-15s 0x%016llX\n",
        "Created:",
        (unsigned long long)record->metadata.fn.creation_time);
    
    printf("    %-15s 0x%016llX\n",
        "Modified:",
        (unsigned long long)record->metadata.fn.modified_time);
    
    printf("    %-15s 0x%016llX\n",
        "MFT Modified:",
        (unsigned long long)record->metadata.fn.mft_modified_time);

    printf("    %-15s 0x%016llX\n",
        "Accessed:",
        (unsigned long long)record->metadata.fn.accessed_time);

    printf("\n");


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