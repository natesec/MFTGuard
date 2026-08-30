#ifndef MFT_H
#define MFT_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "attributes.h"

#define ERROR_MARKER "[!] "
#define SUCCESS_MARKER "[+] "
#define MESSAGE_MARKER "[*] "

#define MFT_DEFAULT_RECORD_SIZE 1024
#define MFT_RECORD_HEADER_SIZE 0x30
#define MFT_RECORD_ALLOCATED_SIZE_OFFSET 0x1C
#define MFT_RECORD_ALLOCATED_SIZE_END    0x20

#define MFT_SIGNATURE_FILE "FILE"  // 0x454C4946
#define MFT_SIGNATURE_BAAD  "BAAD" //0x44414142

/**
 * @brief Represents an open $MFT file and its state.
 */
typedef struct
{
    FILE* fptr;             /** File descriptor for the $MFT. */
    uint64_t file_size;     /** Size of the $MFT file in bytes. */
    uint8_t *buffer;        /** Buffer containing the current raw record. */
    uint64_t record_size;   /** Size of each record in bytes. */
    uint64_t record_number; /** Index of the current record. */
    uint64_t record_count;  /** Total number of records. */
} mft_file;

/**
 * @brief Represents an NTFS FILE record header in the $MFT.
 */
typedef struct
{
    char signature[4];          /** 0x00 - "FILE" (valid) "BAAD" (invalid). */
    uint16_t usa_offset;        /** 0x04 - Fixup array offset. */
    uint16_t usa_count;         /** 0x06 - Number of entries in the fixup array */
    uint64_t lsn;               /** 0x08 - Log sequence number. */
    uint16_t sequence_number;   /** 0x10 - Seqnum for reuse detection. */
    uint16_t hard_link_count;   /** 0x12 - Hard link counting. */
    uint16_t attribute_offset;  /** 0x14 - Offset to first attribute. */
    uint16_t flags;             /** 0x16 - Status indicators. */
    uint32_t used_size;         /** 0x18 - Bytes used in this record. */
    uint32_t allocated_size;    /** 0x1C - Total size in bytes of this record. */
    uint64_t base_record;       /** 0x20 - Base file record segment reference. */
    uint16_t next_attribute_id; /** 0x28 - Incremental identifier. */
    uint16_t alignment;         /** 0x2A - Align or unused. */
    uint32_t record_number;     /** 0x2C - MFT record number. */
} mft_record_header;

/**
 * @brief Represents the result of parsing an MFT record.
 */
typedef enum
{
    MFT_RECORD_INVALID,
    MFT_RECORD_UNUSED,
    MFT_RECORD_USED
} mft_record_status;

/**
 * @brief Represents a parsed MFT record.
 */
typedef struct
{
    uint64_t record_number;   /** Index of the MFT record. */
    mft_record_header header; /** Populated MFT record header. */
    record_metadata metadata; /** Parsed attributes for analysis */
} mft_record;

/** 
 * @brief Opens a binary $MFT file for reading.
 * @param mft Pointer to the mft_File structure to initialize.
 * @param path Path to the $MFT file.
 * @return true if the file was opened, false otherwise.
 */
bool mft_open(mft_file *mft, const char *path);

/**
 * @brief Reads the current MFT record into the buffer.
 * @param mft Pointer to the declared mft_file structure.
 * @return true if a complete record was read, false otherwise.
 */
bool mft_read_record(mft_file *mft);

/**
 * @brief Parses an MFT record.
 * @param mft Pointer to an mft_file structure containing a record in mft->buffer.
 * @param record Pointer to the current record ready for parsing.
 * @param sector_size Size of the disk sectors (declared in $Boot).
 * @return true if the record was parsed and is valid, false otherwise. 
 */
mft_record_status mft_parse_record(mft_file *mft, mft_record *record, uint32_t sector_size);

/**
 * @brief Closes the MFT file and frees allocated buffer.
 * @param mft Pointer to a declared mft_file structure to clean up.
 */
void mft_close(mft_file *mft);

#endif