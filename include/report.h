#ifndef REPORT_H
#define REPORT_H

#include <stdint.h>
#include <stdbool.h>

#include "mft.h"
#include "statistics.h"
#include "cJSON.h"
#include "candidate.h"

/**
 * @brief Represents a cJSON root object.
 */
typedef struct
{
    cJSON *root;
} report;


/**
 * @brief Create and output a report on a suspicious MFT record.
 * @param record Pointer to a suspicious record.
 * @param rule_flags Bitmask containing the detection results from rules.
 */
void report_record(const mft_record *record, uint32_t rule_flags);

/**
 * @brief Initialize cJSON reporting.
 * @param report Pointer to the cJSON report structure containing the root.
 */
bool report_initialize(report *report);

/**
 * @brief Add MFT statistics and information to the cJSON object.
 * @param report Pointer to the cJSON report structure.
 * @param stats Pointer to the populated statistics struct.
 */
bool report_add_overview(report *report, const statistics *stats);

/**
 * @brief Recursively free everything in the cJSON object under root.
 * @param report Pointer to the report structure to be cleared.
 */
void report_free(report *report);

/**
 * @brief Write overview, then call candidate helper to write to the specified JSON file.
 * @param report Pointer to the report struct containing the root.
 * @param candidates Pointer to the populated candidate hash table.
 * @param filename Pointer to the filename to write to.
 * @return true if successfully wrote all candidates, false otherwise.
 */
bool report_write(const report *report, const candidate *candidates, const char *filename);

#endif