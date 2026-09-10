#include <stdlib.h>
#include <string.h>

#include "candidate.h"

/**
 * @brief Copy the filename from a filename attribute field into candidate memory.
 * @param destination pointer to the candidate file_name_information struct.
 * @param source pointer to the record_metadata file_name_information struct.
 * @return true if the filename was copied successfully, false otherwise.
 */
static bool candidate_copy_filename(
    file_name_information *destination,
    const file_name_information *source
);

/**
 * @brief Copy all file name structs into candidate memory, deep-copy file name field.
 * @param candidate pointer to the candidate struct.
 * @param metadata pointer to the record_metadata struct.
 * @return true if successful copy, false otherwise.
 */
static bool candidate_copy_file_names(candidate *candidate, record_metadata *metadata);

bool candidate_add(
    candidate **candidates,
    const record_metadata *metadata,
    uint32_t rule_flags)
{
    return true;
}

void candidate_free_all(candidate **candidates)
{

}

static bool candidate_copy_filename(
    file_name_information *destination,
    const file_name_information *source)
{
    if (destination == NULL || source == NULL)
    {
        return false;
    }

    if (source->filename == NULL || source->filename_length == NULL)
    {
        return false;
    }

    size_t filename_size = (size_t)source->filename_length * 2;

    uint8_t *filename_copy = malloc(filename_size);

    if (filename_copy == NULL)
    {
        return false;
    }

    memcpy(filename_copy, source->filename, filename_size);

    destination->filename = filename_copy;

    return true;
}

static bool candidate_copy_file_names(candidate *candidate, record_metadata *metadata)
{
    if (candidate == NULL || metadata == NULL)
    {
        return false;
    }

    candidate->file_name_count = metadata->file_name_count;

    if (metadata->file_name_count == 0)
    {
        candidate->file_names = NULL;
        return true;
    }

    candidate->file_names = malloc((size_t)metadata->file_name_count * sizeof(file_name_information));

    if (candidate->file_names == NULL)
    {
        return false;
    }

    for (uint32_t i = 0; i < metadata->file_name_count; i++)
    {
        candidate->file_names[i] = metadata->file_names[i];

        if (!candidate_copy_filename(&candidate->file_names[i], &metadata->file_names[i]))
        {
            for (uint32_t j = 0; j < i; j++)
            {
                free((void *)candidate->file_names[j].filename);
            }

            free(candidate->file_names);
            candidate->file_names = NULL;
            candidate->file_name_count = 0;

            return false;
        }
    }

    return true;
}