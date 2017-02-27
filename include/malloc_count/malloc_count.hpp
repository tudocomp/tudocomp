#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <cstdint>

using ulong = unsigned long;

/// \cond INTERNAL

/// \brief Contains a custom override of \c malloc used to count allocated
/// memory over time.
namespace malloc_count {

    inline ulong current_time_millis() {
        timespec t;
        clock_gettime(CLOCK_MONOTONIC, &t);

        return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
    }

    struct phase_t {
        size_t id;

        ulong time_start;
        ulong time_end;
        ssize_t mem_off;
        ssize_t mem_current;
        ssize_t mem_peak;
    };

    void begin_phase();
    phase_t end_phase();
    void pause_phase();
    void resume_phase();
}

#ifndef __CYGWIN__ //these are not defined in Cygwin

extern "C" void* __libc_malloc(size_t);
extern "C" void __libc_free(void*);
extern "C" void* __libc_realloc(void*, size_t);

#endif

/// \endcond

