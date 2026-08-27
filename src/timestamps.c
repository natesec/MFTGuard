#include <stdio.h>

#include "timestamps.h"

bool timestamp_equal(uint64_t first, uint64_t second)
{
    return first == second;
}

bool timestamp_is_before(uint64_t first, uint64_t second)
{
    return first < second;
}

bool timestamp_is_after(uint64_t first, uint64_t second)
{
    return first > second;
}

/** TODO: implement after filetime to datetime conversion is complete */
bool timestamp_low_digits_zeroed(uint64_t timestamp)
{
    return true;
}