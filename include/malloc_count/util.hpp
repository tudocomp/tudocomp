#ifndef _INCLUDED_TUDOSTAT_UTIL_HPP_
#define _INCLUDED_TUDOSTAT_UTIL_HPP_

#include <time.h>

using ulong = unsigned long;

namespace tudostat {

    inline ulong current_time_millis() {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

}

#endif

