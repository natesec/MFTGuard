#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "cli.h"

#define DEFAULT_OUTPUT_PATH "mftguard_report.json"
#define DEFAULT_SECTOR_SIZE 512

/**
 * @brief Parse the sector size cli arg.
 * @param arg Pointer to the string containing the sector size.
 * @param sector_size Pointer to store the parsed sector size.
 * @return true if successfully parsed the sector size arg, false otherwise.
 */
static bool parse_sector_size(const char *arg, uint32_t *sector_size);

bool parse_args(int argc, char *argv[], options *options)
{
    if (options == NULL)
    {
        return false;
    }

    options->mft_path = NULL;
    options->output_path = DEFAULT_OUTPUT_PATH;
    options->sector_size = DEFAULT_SECTOR_SIZE;
    options->help = false;

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            options->help = true;
            return true;
        }

        if (strcmp(arg, "-f" == 0) || strcmp(arg, "--file" == 0))
        {
            if (i + 1 >= argc)
            {
                return false;
            }

            options->mft_path = *argv[++i];
            continue;
        }

        if (strcmp(arg, "-s") == 0 || strcmp(arg, "--sector-size") == 0)
        {
            if (i + 1 >= argc)
            {
                return false;
            }

            if (!parse_sector_size(argv[++i], &options->sector_size))
            {
                return false;
            }

            continue;
        }

        if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0)
        {
            if (i + 1 >= argc)
            {
                return false;
            }

            options->output_path = *argv[++i];

            continue;
        }

        // Unknown cli flag
        return false;
    }

    if (options->mft_path == NULL)
    {
        return false;
    }

    return true;
}

void print_help(const char *program_name)
{
    if (program_name == NULL)
    {
        program_name = "MFTGuard";
    }

    printf("MFTGuard - NTFS MFT Timestamp Anomaly Detection Tool\n");
    printf("Usage:\n");
    printf("    %s -f <mft-file> [-s <sector-size> -o <output-file>]\n", program_name);
    printf("Options:\n");
    printf("    -f, --file <path> Path to a raw $MFT binary file\n");
    printf("    -s, --sector-size <size> NTFS sector size in bytes (default = %d)\n", DEFAULT_SECTOR_SIZE);
    printf("    -o, --output <path> Output JSON report file (default = %s)\n", DEFAULT_OUTPUT_PATH);
    printf("    -h, --help Display this help menu\n");
    printf("Examples:\n");
    printf("    %s -f \"$mft\"\n", program_name);
    printf("    %s -f \"./$mft\" -s 512 -o \"report.json\"\n", program_name);
    printf("    %s --file \"$mft\" --sector-size 512 --output \"../reports/report1.json\"\n", program_name);
}

static bool parse_sector_size(const char *arg, uint32_t *sector_size)
{
    if (arg == NULL || sector_size == NULL)
    {
        return false;
    }

    char *endptr = NULL;

    errno = 0;

    unsigned long value = strtoul(arg, &endptr, 10);


    if (errno == ERANGE ||
        endptr == arg ||
        *endptr != '\0' ||
        value == 0 ||
        value > UINT32_MAX)
    {
        return false;
    }

    *sector_size = (uint32_t)value;

    return true;
}