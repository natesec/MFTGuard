#ifndef CLI_H
#define CLI_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Represents the cli options passed to main.
 */
typedef struct
{
    const char *mft_path;
    const char *output_path;
    uint32_t sector_size;
    bool help;
} options;

/**
 * @brief Parse and validate cli arguments.
 * @param argc Number of cli arguments.
 * @param argv Pointer to the arguments array.
 * @param options Pointer to the options struct to populate.
 * @return true if successfully parsed and validated cli args, false otherwise.
 */
bool parse_args(int argc, char *argv[], options *options);

/**
 * @brief Print cli usage and options.
 * @param program_name Pointer to the name of the exe.
 */
void print_help(const char *program_name);

#endif