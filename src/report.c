#include <stdint.h>
#include <stdio.h>

#include "report.h"
#include "rules.h"

#define RULE_MARKER "----"

void report_record(const mft_record *record, uint32_t rule_flags)
{
    if (record == NULL || rule_flags == RULE_NONE)
    {
        return;
    }

    printf("\n");
    printf(MESSAGE_MARKER "SUSPICIOUS RECORD: %llu\n", (unsigned long long)record->record_number);

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