#include <stdio.h>

#include "timestamps.h"

#define ERROR_MARKER "[!] "
#define FILETIME_TICKS_PER_SECOND 10000000ULL

// Seconds since Jan 1st, 1970, UNIX time begins here
#define FILETIME_UNIX_EPOCH_OFFSET 11644473600ULL

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

bool filetime_to_utc(uint64_t filetime, struct tm *result)
{
    if (result == NULL)
    {
        return false;
    }

    uint64_t filetime_seconds = filetime / FILETIME_TICKS_PER_SECOND;

    if (filetime_seconds < FILETIME_UNIX_EPOCH_OFFSET)
    {
        fprintf(stderr, ERROR_MARKER "filetime/utc conversion failed: filetime precedes UTC\n");
        return false;
    }

    uint64_t unix_seconds = filetime_seconds - FILETIME_UNIX_EPOCH_OFFSET;
    
    time_t unix_time = (time_t)unix_seconds;

    if ((uint64_t)unix_time != unix_seconds)
    {
        fprintf(stderr, ERROR_MARKER "filetime/utc conversion failed: time_t cast unsuccessful\n");
        return false;
    }

    // Detect compiler
#if defined(_MSC_VER)
    if (gmtime_s(result, &unix_time) != 0)
    {
        return false;
    }
#else
    struct tm *utc_time = gmtime(&unix_time);

    if (utc_time == NULL)
    {
        return false;
    }

    *result = *utc_time;
#endif

    return true;
}