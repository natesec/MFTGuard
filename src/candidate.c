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