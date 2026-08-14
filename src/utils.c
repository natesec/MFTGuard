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