#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/** NTFS attribute type that marks the end of the attribute list. */
#define ATTRIBUTE_TYPE_END 0xFFFFFFFF

#define ATTRIBUTE_TYPE_STANDARD_INFORMATION 0x10
#define ATTRIBUTE_TYPE_FILE_NAME 0x30
#define ATTRIBUTE_TYPE_DATA 0x80
#define ATTRIBUTE_TYPE_BITMAP 0xB0

/** TODO: update these values once more info is parsed */
#define STANDARD_INFORMATION_TIMESTAMP_SIZE 0x20
#define FILE_NAME_TIMESTAMP_SIZE 0x28

/**
 * @brief Represents an MFT record attribute header.
 */
typedef struct
{
    uint32_t type;         /** 0x00 */
    uint32_t length;       /** 0x04 */
    uint8_t non_resident;  /** 0x08 */
    uint8_t name_length;   /** 0x09 */
    uint16_t name_offset;  /** 0x0A */
    uint16_t flags;        /** 0x0C */
    uint16_t attribute_id; /** 0x0E */
} attribute_header;

/**
 * @brief Represents a resident attribute header.
 * @note The common attribute header occupies 0x00–0x0F, so the resident header starts at 0x10.
 */
typedef struct
{
    uint32_t value_length; /** 0x00 */
    uint16_t value_offset; /** 0x04 */
    uint8_t indexed;       /** 0x06 */
    uint8_t padding;       /** 0x07 */
} resident_attribute_header;

/**
 * @brief Represents an MFT record $STANDARD_INFORMATION attribute type.
 */
typedef struct
{
    uint64_t creation_time;     /** 0x00 */
    uint64_t modified_time;     /** 0x08 */
    uint64_t mft_modified_time; /** 0x10 */
    uint64_t accessed_time;     /** 0x18 */
} standard_information;

/**
 * @brief Represents an MFT record $FILE_NAME attribute type.
 */
typedef struct
{
    uint64_t parent_directory;  /** 0x00 */
    uint64_t creation_time;     /** 0x08 */
    uint64_t modified_time;     /** 0x10 */
    uint64_t mft_modified_time; /** 0x18 */
    uint64_t accessed_time;     /** 0x20 */
    uint64_t allocated_size;    /** 0x28 */
    uint64_t used_size;         /** 0x30 */
    uint32_t flags;             /** 0x38 */
    uint32_t reparse_and_ea;    /** 0x3C */
    uint8_t filename_length;    /** 0x40 */
    uint8_t filename_namespace; /** 0x41 */
    const uint8_t *filename;
    bool is_usable;
} file_name_information;

/**
 * @brief Used to store parsed attribute data for a record.
 */
typedef struct
{
    uint64_t record_number;

    bool has_standard_information;
    standard_information si;

    file_name_information *file_names;
    uint32_t file_name_count;
    uint32_t file_name_capacity;
} record_metadata;

/**
 * @brief Walks the attributes for a given MFT record.
 * @param record Pointer to the record with attributes to walk.
 * @param record_size Size of the record in bytes.
 * @param attribute_offset Offset to the first attribute.
 * @return true if the attribute list was walked, false otherwise.
 */
bool attributes_walk(
    const uint8_t *record,
    uint32_t record_size,
    uint16_t attribute_offset,
    record_metadata *metadata
);

/**
 * @brief Establish a known empty state for record_metadata struct.
 * @param metadata Pointer to the metadata struct to initialize.
 */
void record_metadata_init(record_metadata *metadata)
{
    if (metadata == NULL)
    {
        return;
    }

    metadata->record_number = 0;
    metadata->has_standard_information = false;
    metadata->si = (standard_information){0};

    metadata->file_names = NULL;
    metadata->file_name_count = 0;
    metadata->file_name_capacity = 0;
}

/**
 * @brief Free memory allocated to record_metadata fields.
 * @param metadata Pointer to the metadata struct to free.
 */
void record_metadata_free(record_metadata *metadata)
{
    if (metadata == NULL)
    {
        return;
    }

    free(metadata->file_names);

    metadata->file_names = NULL;
    metadata->file_name_count = 0;
    metadata->file_name_capacity = 0;
}

/**
 * @brief Add a new file_name_information struct to record_metadata.
 * @param metadata Pointer to the record metadata struct to add the file name to.
 * @param fn Pointer to the file name information struct to add to record metadata.
 * @return true if successfully added file name, false otherwise.
 */
bool record_metadata_add_file_name(record_metadata *metadata, const file_name_information *fn)
{
    if (metadata == NULL || fn == NULL)
    {
        return false;
    }

    if (metadata->file_name_count >= metadata->file_name_capacity)
    {
        uint32_t new_capacity;

        if (metadata->file_name_capacity == 0)
        {
            new_capacity = 1;
        }
        else
        {
            if (metadata->file_name_capacity > UINT32_MAX / 2)
            {
                return false;
            }

            new_capacity = metadata->file_name_capacity * 2;
        }

        file_name_information *new_file_names = realloc(
            metadata->file_names,
            (size_t)new_capacity * sizeof(file_name_information)
        );

        if (new_file_names == NULL)
        {
            return false;
        }

        metadata->file_names = new_file_names;
        metadata->file_name_capacity = new_capacity;
    }

    metadata->file_names[metadata->file_name_count] = *fn;
    metadata->file_name_count++;

    return true;
}

#endif