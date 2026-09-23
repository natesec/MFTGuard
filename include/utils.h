#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/**
 * @brief Detects whether local machine handles byte
 *        order in little-endian.
 * @note Solely for sanity checks.
 * @return true if little-endian, false if big-endian.
 */
bool is_little_endian(void);

/**
 * @brief Translates uint16 values from little-endian.
 * @param buffer Pointer to the data buffer.
 * @note Explicitly interpret the bytes as little-endian
 *       regardless of the host architecture.
 * @return Translated uint16_t value. 
 */
uint16_t read_u16_le(const uint8_t *buffer);

/**
 * @brief Translates uint32 values from little-endian.
 * @param buffer Pointer to the data buffer.
 * @note Explicitly interpret the bytes as little-endian
 *       regardless of the host architecture.
 * @return Translated uint32_t value. 
 */
uint32_t read_u32_le(const uint8_t *buffer);

/**
 * @brief Translates uint64 values from little-endian.
 * @param buffer Pointer to the data buffer.
 * @note Explicitly interpret the bytes as little-endian
 *       regardless of the host architecture.
 * @return Translated uint64_t value. 
 */
uint64_t read_u64_le(const uint8_t *buffer);

/**
 * @brief Convert a utf16le WCHAR string to UTF-8.
 * @param input Pointer to the UTF16-LE input string.
 * @param length Length of the input string.
 * @return Pointer to the converted UTF-8 string.
 */
char *utf16le_to_utf8(const uint8_t *input, uint8_t length);

#endif