#include <stdint.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <Windows.h>
#elif defined(__linux__)
    #include <iconv.h>
#endif

#include "utils.h"


/** 
 * Compare "first byte" of 0x0001 in memory
 * On little-endian: 01
 * On big-endian:    00
 */
bool is_little_endian(void)
{
    uint16_t value = 0x0001;

    return *((uint8_t *)&value) == 0x01;
}

/**
 * Buffer[0] = least significant byte
 * Buffer[1] = Most significant byte
 * Shift buffer[1] bytes to least significant position
 * Bitwise OR to merge bits in both positions
 */
uint16_t read_u16_le(const uint8_t *buffer)
{
    return (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
}

/**
 * Interprets little-endian bytes
 */
uint32_t read_u32_le(const uint8_t *buffer)
{
    return (uint32_t)buffer[0] 
        | ((uint32_t)buffer[1] << 8) 
        | ((uint32_t)buffer[2] << 16) 
        | ((uint32_t)buffer[3] << 24);
}

/**
 * Interprets little-endian bytes
 */
uint64_t read_u64_le(const uint8_t *buffer)
{
    return (uint64_t)buffer[0] 
        | ((uint64_t)buffer[1] << 8) 
        | ((uint64_t)buffer[2] << 16) 
        | ((uint64_t)buffer[3] << 24)
        | ((uint64_t)buffer[4] << 32)
        | ((uint64_t)buffer[5] << 40)
        | ((uint64_t)buffer[6] << 48)
        | ((uint64_t)buffer[7] << 56);
}

char *utf16le_to_utf8(const uint8_t *input, uint8_t length)
{
#ifdef _WIN32
    if (input == NULL || length == 0)
    {
        return NULL;
    }

    int utf8_length = WideCharToMultiByte(
        CP_UTF8,
        0,
        (const wchar_t *)input,
        length,
        NULL,
        0,
        NULL,
        NULL
    );

    if (utf8_length <= 0)
    {
        return NULL;
    }

    char *output = malloc((size_t)utf8_length + 1);

    if (output == NULL)
    {
        return NULL;
    }

    int result = WideCharToMultiByte(
        CP_UTF8,
        0,
        (const wchar_t *)input,
        length,
        output,
        utf8_length,
        NULL,
        NULL
    );

    if (result != utf8_length)
    {
        free(output);
        return NULL;
    }

    output[utf8_length] = '\0';

    return output;
#elif defined(__linux__)
    iconv_t cd = iconv_open("UTF-8", "UTF-16LE");

    if (cd == (iconv_t)-1)
    {
        return NULL;
    }

    size_t input_bytes = length;
    size_t output_bytes = (length * 2) + 1;

    char *output = malloc(output_bytes);

    if (output == NULL)
    {
        iconv_close(cd);
        return NULL;
    }

    char *input_ptr = (char *)input;
    char *output_ptr = output;
    size_t remaining_output = output_bytes - 1;

    if (iconv(cd, &input_ptr, &input_bytes, &output_ptr, &remaining_output) == (size_t)-1)
    {
        free(output);
        iconv_close(cd);
        return NULL;
    }

    *output_ptr = '\0';

    iconv_close(cd);

    return output;
#else
    return NULL;
#endif
}