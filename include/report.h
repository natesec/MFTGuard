#ifndef REPORT_H
#define REPORT_H

#include <stdint.h>

#include "mft.h"

/**
 * @brief Create and output a report on a suspicious MFT record.
 * @param record Pointer to a suspicious record.
 * @param rule_flags Bitmask containing the detection results from rules.
 */
void report_record(const mft_record *record, uint32_t rule_flags);

#endif